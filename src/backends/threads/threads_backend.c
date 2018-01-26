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
  mtx_t mtx_c;
  mtx_t mtx_d;

  cnd_t cnd_a;
  cnd_t cnd_b;
//  cnd_t cnd_c;
} _thread_ctx;

#define CTX(__h) (_thread_ctx*)(__h)->a;

static int _thread_runner(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);

  cnd_wait(&ctx->cnd_a, &ctx->mtx_a);

//  mtx_lock(&ctx->mtx_c);
//  mtx_lock(&ctx->mtx_a);

  h->fn(h->ctx);

  cnd_signal(&ctx->cnd_b); // make it run

  h->finished = true;

  return 0;
}

void _koro_backend_init(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = (_thread_ctx*)malloc(sizeof(_thread_ctx));

  mtx_init(&ctx->mtx_a, mtx_plain);
  mtx_init(&ctx->mtx_b, mtx_plain);
  mtx_init(&ctx->mtx_c, mtx_plain);
  mtx_init(&ctx->mtx_d, mtx_plain);
//  mtx_init(&ctx->mtx_c, mtx_plain);
  cnd_init(&ctx->cnd_a);
  cnd_init(&ctx->cnd_b);
//  cnd_init(&ctx->cnd_c);

//  mtx_lock(&ctx->mtx_a);

  thrd_create(&ctx->thrd, _thread_runner, h);
//  thrd_detach(&ctx->thrd);

  h->a = (uintptr_t)ctx;
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  _thread_ctx * ctx = CTX(h);

  cnd_signal(&ctx->cnd_a); // make it run

  //mtx_lock(&ctx->mtx_d);
  cnd_wait(&ctx->cnd_b, &ctx->mtx_b); // wait until yield
  //mtx_unlock(&ctx->mtx_c);


}

void _koro_yield(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = CTX(h);

//  mtx_lock(&ctx->mtx_b);
//  mtx_unlock(&ctx->mtx_yield);

//  while(!ctx->cnd_b.mWaitersCount) {}

  cnd_signal(&ctx->cnd_b);

  cnd_wait(&ctx->cnd_a, &ctx->mtx_a);

//  cnd_signal(&ctx->cnd_c);

  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = 10000000;
//  thrd_sleep(&t, NULL);

}

#endif
