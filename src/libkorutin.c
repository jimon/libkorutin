#include <libkorutin.h>
#include "scheduling.h"
#include "backend.h"

// -------------------------------------------------------------------------------------- root functions

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
  #ifdef KORO_WATERMARKING
  for(uint32_t * ptr = (uint32_t*)h->stack_end; ptr < (uint32_t*)h->stack_start; ++ptr)
    *ptr = 0xfee1dead;
  #endif
  _koro_backend_init(h);
}

void koro_run(koro_t * h)
{
  if(_koro_get_current())
    return;

  _koro_set_current(h);
  _koro_run(h);
  _koro_set_current(NULL);
}

void koro_yield(void)
{
  if(!_koro_get_current())
    return;
  _koro_yield(_koro_get_current());
}

// -------------------------------------------------------------------------------------- stack usage inspection

#ifdef KORO_WATERMARKING
size_t koro_calculate_stack_watermark(koro_t * h)
{
  if(!h)
    return 0;

  for(uint32_t * ptr = (uint32_t*)h->stack_end; ptr < (uint32_t*)h->stack_start; ++ptr)
    if((*ptr) != 0xfee1dead)
      return (size_t)((uint8_t*)h->stack_start - (uint8_t*)ptr);

  return 0;
}
#endif
