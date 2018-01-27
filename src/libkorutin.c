#include <libkorutin.h>
#include "scheduling.h"
#include "debug.h"
#include "backend.h"

#include <assert.h>

// -------------------------------------------------------------------------------------- root functions

const uint32_t koro_magic = 0xdeadfee1;
const uint32_t koro_watermarking_const = 0xfee1dead;

static void * align_ptr_low(void * ptr)
{
  return (void*)(((uintptr_t)ptr) & ~(KORO_STACK_ALIGNMENT - 1));
}

void koro_init(koro_t * h, koro_func_t fn, void *ctx, uint8_t *stack_mem, size_t stack_mem_size)
{
  if(!h || !fn || stack_mem_size < sizeof(uint32_t))
    return;
  h->finished = false;
  h->fn = fn;
  h->ctx = ctx;
  h->stack_mem = stack_mem;
  #ifdef KORO_EXTERNAL_STACK_AWARE
    h->stack_end = stack_mem;
    h->stack_start = align_ptr_low(stack_mem + stack_mem_size - 1); // stack grows down
    #ifdef KORO_WATERMARKING
      for(uint32_t * ptr = (uint32_t*)h->stack_end; ptr < (uint32_t*)h->stack_start; ++ptr)
        *ptr = koro_watermarking_const;
    #endif
  #else
    (void)stack_mem;
    (void)stack_mem_size;
    h->stack_end = NULL;
    h->stack_start = NULL;
  #endif
  _koro_backend_init(h);
  h->magic = koro_magic;
}

void koro_run(koro_t * h)
{
  assert(h->magic == koro_magic);

  if(_koro_get_current())
    return;

  _koro_set_current(h);

  #ifdef KORO_SET_HARDWARE_BREAKPOINTS
    _koro_set_hw_break_at(h->stack_end);
  #endif

  _koro_run(h);

  #ifdef KORO_SET_HARDWARE_BREAKPOINTS
    _koro_clear_hw_break(h->stack_end);
  #endif

  _koro_set_current(NULL);
}

void koro_yield(void)
{
  if(!_koro_get_current())
    return;

  koro_t * h = _koro_get_current();
  assert(h->magic == koro_magic);

  #ifdef KORO_EXTERNAL_STACK_AWARE
    #ifdef KORO_WATERMARKING
      // check that final stack pointer is still watermarked
      // otherwise there is a huge chance that stack was overflown
      assert(*(uint32_t*)h->stack_end == koro_watermarking_const);
    #endif
  #endif

  _koro_yield(h);
}

// -------------------------------------------------------------------------------------- stack usage inspection

#ifdef KORO_WATERMARKING
size_t koro_calculate_stack_watermark(koro_t * h)
{
  if(!h)
    return 0;

  #ifdef KORO_EXTERNAL_STACK_AWARE
    for(uint32_t * ptr = (uint32_t*)h->stack_end; ptr < (uint32_t*)h->stack_start; ++ptr)
      if((*ptr) != koro_watermarking_const)
        return (size_t)((uint8_t*)h->stack_start - (uint8_t*)ptr);
  #endif

  return 0;
}
#endif
