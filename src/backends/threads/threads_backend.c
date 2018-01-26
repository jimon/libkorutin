#include <libkorutin.h>

#ifdef KORO_BACKEND_THREADS

// this is a very slow backend, and it uses native thread handles
// so it might be a good idea to keep amount of running coroutines low (like <100)

// this backend might be useful in case where asm-based backend doesn't work
// (ABI changes, unsupported instruction set, etc) and you need to ship something NOW

// this will work everywhere where tinycthread work, which is all Windows platforms and all POSIX platforms

#include "backend.h"
#include "tinycthread.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct
{
  thrd_t thrd;
  mtx_t mtx_a;
  mtx_t mtx_b;
  cnd_t cnd_a;
  cnd_t cnd_b;
} _thread_ctx;

#define CTX(__h) (_thread_ctx*)(__h)->a;

static int _thread_runner(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);
  cnd_wait(&ctx->cnd_a, &ctx->mtx_a);
  h->fn(h->ctx);
  h->finished = true;
  cnd_signal(&ctx->cnd_b);
  thrd_exit(0);
  return 0;
}

void _koro_backend_init(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = (_thread_ctx*)malloc(sizeof(_thread_ctx));

  mtx_init(&ctx->mtx_a, mtx_plain);
  mtx_init(&ctx->mtx_b, mtx_plain);
  cnd_init(&ctx->cnd_a);
  cnd_init(&ctx->cnd_b);
  thrd_create(&ctx->thrd, _thread_runner, h);
  h->a = (uintptr_t)ctx;
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  _thread_ctx * ctx = CTX(h);

  cnd_signal(&ctx->cnd_a);
  cnd_wait(&ctx->cnd_b, &ctx->mtx_b);

  if(h->finished)
  {
    cnd_destroy(&ctx->cnd_a);
    cnd_destroy(&ctx->cnd_b);
    mtx_destroy(&ctx->mtx_a);
    mtx_destroy(&ctx->mtx_b);
    free(ctx);
    h->ctx = NULL;
  }
}

void _koro_yield(koro_t * h)
{
  if(!h)
    return;

  thrd_yield();

  _thread_ctx * ctx = CTX(h);
  cnd_signal(&ctx->cnd_b);
  cnd_wait(&ctx->cnd_a, &ctx->mtx_a);
}

#endif
