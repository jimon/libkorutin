#include "libkorutin.h"
#include <assert.h>

// -------------------------------------------------------------------------------------- stack switching

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

typedef void *(*stack_op_t)(void * stack_ptr, koro_t * extra);
// must not inline, otherwise we screw our stack fold/unfold
STATIC_NOINLINE void *_switch_stack(stack_op_t save_state, stack_op_t restore_state, koro_t * h)
{
  typedef void *(*switch_op_t)(void*, void*);
  return slp_switch((switch_op_t)save_state, (switch_op_t)restore_state, h);
}

// --------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------

void _koro_backend_init(koro_t * h)
{
  // no op
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  if(h->_stack_run)
    _switch_stack(_run_swap, _restore_noop, h);
  else
    _switch_stack(_init_swap, _init_run, h);
}

void _koro_yield(koro_t * h)
{
  if(!h)
    return;

  // sanity check
  uint8_t byte_on_stack = 0;
  volatile void * ptr = &byte_on_stack;
  assert(h->stack_end <= ptr && h->stack_start >= ptr);

  _switch_stack(_yield_swap, _yield_cleanup, h);
}

