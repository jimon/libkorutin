#include <libkorutin.h>

#ifdef KORO_BACKEND_THREADS

// this is a very slow backend, and it uses native thread handles
// so it might be a good idea to keep amount of running coroutines low (like <50)

// this backend might be useful in case where asm-based backend doesn't work
// (ABI changes, unsupported instruction set, etc) and you need to ship something NOW

// this will work everywhere where tinycthread work, which is all Windows platforms and all POSIX platforms

#include "backend.h"
#include "tinycthread.h"

#include <stdlib.h>
#include <stdio.h>

#include <signal.h>

typedef struct
{
  thrd_t thrd;

  sig_atomic_t wait_run;
  sig_atomic_t wait_yield;
} _thread_ctx;

#define CTX(__h) (_thread_ctx*)(__h)->a;

// TODO wait on spinlocks do not scale at all, so performance degrades a lot after 10 spinlocks already
#define WAIT_UNTIL(__expr) while(!(__expr)) {thrd_yield();}

static void _wait_for_yield_continue(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);

  ctx->wait_yield = true;
  WAIT_UNTIL(!ctx->wait_yield);
}

static void _continue_yield(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);
  WAIT_UNTIL(ctx->wait_yield);
  ctx->wait_yield = false;
}

static void _wait_for_run_continue(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);

  ctx->wait_run = true;
  WAIT_UNTIL(!ctx->wait_run);
}

static void _continue_run(koro_t * h)
{
  _thread_ctx * ctx = CTX(h);

  WAIT_UNTIL(ctx->wait_run);
  ctx->wait_run = false;
}

static int _thread_runner(koro_t * h)
{
  _wait_for_yield_continue(h);
  h->fn(h->ctx);
  h->finished = true;
  _continue_run(h);
  thrd_exit(0);
  return 0;
}

void _koro_backend_init(koro_t * h)
{
  if(!h)
    return;

  _thread_ctx * ctx = (_thread_ctx*)malloc(sizeof(_thread_ctx));
  h->a = (uintptr_t)ctx;
  ctx->wait_run = false;
  ctx->wait_yield = false;
  thrd_create(&ctx->thrd, (thrd_start_t)_thread_runner, h);
}

void _koro_run(koro_t * h)
{
  if(!h || h->finished)
    return;

  _thread_ctx * ctx = CTX(h);

  _continue_yield(h);
  _wait_for_run_continue(h);

  if(h->finished)
  {
    free(ctx);
    h->ctx = NULL;
  }
}

void _koro_yield(koro_t * h)
{
  if(!h)
    return;

  _continue_run(h);
  _wait_for_yield_continue(h);
}

#endif
