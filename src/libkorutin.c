#include "libkorutin.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

// ---------------------------------------------------- stack switching

/*
slp_switch is part of pypy stacklet implementation

  The MIT License

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without
  restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or
  sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

// skip the restore if the return value is null
#define STATIC_NOINLINE __attribute__((noinline)) static
#include <slp_platformselect.h>

typedef void *(*stack_op_t)(void*stack_ptr, koro_t*extra);
// must not inline, otherwise we screw our stack fold/unfold
STATIC_NOINLINE void *_switch_stack(stack_op_t save_state, stack_op_t restore_state, koro_t * h)
{
  typedef void *(*switch_op_t)(void*, void*);
  return slp_switch((switch_op_t)save_state, (switch_op_t)restore_state, h);
}

// ---------------------------------------------------- thread aware storage

#if KORO_THREAD_AWARE

/*
_Thread_local is part of tinycthreads

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

      1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

      2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.

      3. This notice may not be removed or altered from any source
      distribution.
*/

  #if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
    #if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
      #define _koro_Thread_local __thread
    #else
      #define _koro_Thread_local __declspec(thread)
    #endif
  #elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
    #define _koro_Thread_local __thread
  #endif

#else
  #define _koro_Thread_local
#endif

// ---------------------------------------------------- basics

static void * align_ptr_low(void * ptr)
{
  return (void*)(((uintptr_t)ptr) & ~(KORO_STACK_ALIGNMENT - 1));
}

void koro_init(koro_t * h, koro_func_t fn, void *ctx, uint8_t *stack_mem, size_t stack_mem_size)
{
  if(!h || !fn)
    return;
  h->finished = false;
  h->fn = fn;
  h->ctx = ctx;
  h->stack_end = stack_mem;
  h->stack_start = align_ptr_low(stack_mem + stack_mem_size - 1); // stack grows down
  h->_stack_org = 0;
  h->_stack_run = 0;
}

static void * _finished_swap(void * stack_old, koro_t * h)
{
  assert(h->stack_end <= stack_old && h->stack_start >= stack_old);
  assert(h->_stack_org != NULL);
  assert(h->_stack_run == NULL);
  return h->_stack_org;
}

static void * _finished_cleanup(void * stack_new, koro_t * h)
{
  assert(h->_stack_org == stack_new);
  assert(h->_stack_run == NULL);

  h->finished = true;
  h->_stack_org = NULL;
  return NULL;
}

static void * _init_swap(void * stack_old, koro_t * h)
{
  assert(h->stack_end != NULL);
  assert(h->stack_start != NULL);
  assert(h->_stack_org == NULL);
  assert(h->_stack_run == NULL);

  h->_stack_org = stack_old;
  return h->stack_start;
}

static void * _init_run(void * stack_new, koro_t * h)
{
  assert(h->stack_start == stack_new);
  assert(h->_stack_org != NULL);
  assert(!h->finished);

  // run coroutine, it will be a first stack frame in this stack
  // so yield will have something to return to
  h->fn(h->ctx);

  // currently mapped sp points to coroutine stack
  // but we have nothing there (coroutine finished it's execution)
  // so we mast force jump to scheduler stack from here
  _switch_stack(_finished_swap, _finished_cleanup, h);

  // this code must never execute
  assert(0);
  return NULL;
}

static void * _run_swap(void * stack_old, koro_t * h)
{
  assert(h->_stack_org == NULL);
  assert(h->_stack_run != NULL);

  h->_stack_org = stack_old;
  return h->_stack_run;
}

static void * _restore_noop(void * stack_new, koro_t * h)
{
  assert(h->_stack_org != NULL);
  assert(h->_stack_run == stack_new);

  h->_stack_run = NULL;
  return NULL;
}

static void * _yield_swap(void * stack_old, koro_t * h)
{
  assert(h->stack_end <= stack_old && h->stack_start >= stack_old);
  assert(h->_stack_org != NULL);
  assert(h->_stack_run == NULL);

  h->_stack_run = stack_old;
  return h->_stack_org;
}

static void * _yield_cleanup(void * stack_new, koro_t * h)
{
  assert(h->_stack_org == stack_new);
  assert(h->_stack_run != NULL);

  h->_stack_org = NULL;
  return NULL;
}

_koro_Thread_local static koro_t * current = NULL;

void koro_run(koro_t * h)
{
  if(!h || h->finished || current)
    return;

  current = h;

  if(h->_stack_run)
    _switch_stack(_run_swap, _restore_noop, h);
  else
    _switch_stack(_init_swap, _init_run, h);

  current = NULL;
}

void koro_yield(void)
{
  if(!current)
    return;

  koro_t * h = current;

  // sanity check
  uint8_t byte_on_stack = 0;
  volatile void * ptr = &byte_on_stack;
  assert(h->stack_end <= ptr && h->stack_start >= ptr);

  _switch_stack(_yield_swap, _yield_cleanup, h);
}
