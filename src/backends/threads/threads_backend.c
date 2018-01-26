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
  mtx_t mtx;
  cnd_t cnd_run_wait;
  cnd_t cnd_yield_wait;
} _thread_ctx;

#define MARK {printf("%s:%u %p\n", __func__, __LINE__, h); fflush(stdout);}
//#define MARK {}

static int _thread_runner(koro_t * h)
{
  _thread_ctx * ctx = (_thread_ctx*)h->_thrd;

  MARK;

  // wait until run unblocks us first time
  cnd_wait(&ctx->cnd_yield_wait, &ctx->mtx);

  MARK;

  h->fn(h->ctx);
  h->finished = true;

  MARK;

  // unblock run
  cnd_signal(&ctx->cnd_run_wait);

  MARK;

  return 0;
}

void _koro_backend_init(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = (_thread_ctx*)malloc(sizeof(_thread_ctx));
  mtx_init(&ctx->mtx, mtx_plain);
  cnd_init(&ctx->cnd_run_wait);
  cnd_init(&ctx->cnd_yield_wait);
  thrd_create(&ctx->thrd, _thread_runner, h);
  h->_thrd = ctx;
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  _thread_ctx * ctx = (_thread_ctx*)h->_thrd;

  MARK;

  // unblock either first run, or yield
  cnd_signal(&ctx->cnd_yield_wait);

  MARK;

  // wait for yield
  cnd_wait(&ctx->cnd_run_wait, &ctx->mtx);

  MARK;

  if(h->finished)
  {
    thrd_join(&ctx->thrd, NULL);
    // clean up
  }
}

void _koro_yield(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = (_thread_ctx*)h->_thrd;

  MARK;

  cnd_signal(&ctx->cnd_run_wait);

  MARK;

  cnd_wait(&ctx->cnd_yield_wait, &ctx->mtx);

  MARK;
}

#endif
