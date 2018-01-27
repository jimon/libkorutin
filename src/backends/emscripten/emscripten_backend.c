
#include <libkorutin.h>

#ifdef KORO_BACKEND_EMSCRIPTEN

#include <emscripten.h>

void _koro_backend_init(koro_t * h)
{
  h->a = (uintptr_t)emscripten_coroutine_create(h->fn, h->ctx, h->stack_mem_size);
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  if(!emscripten_coroutine_next((emscripten_coroutine)h->a))
    h->finished = true;
}

void _koro_yield(koro_t * h)
{
  (void)h;
  emscripten_yield();
}

#endif