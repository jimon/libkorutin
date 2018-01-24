#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define KORO_THREAD_AWARE 0

// coroutine function
// can return at any time, or can yield
typedef void (*koro_func_t)(void * ctx);

// korutin context
typedef struct
{
  // check this after run, if true - coroutine is finished
  // if you call koro_run with finished coroutine - nothing will happen
  bool finished;

  // data
  koro_func_t fn;
  void * ctx;
  void * stack_end;
  void * stack_start;

  // private pointers
  void * _stack_org;
  void * _stack_run;
} koro_t;

#ifdef __cplusplus
extern "C" {
#endif

// initialise context structure
void koro_init(koro_t * h, koro_func_t fn, void * ctx, uint8_t * stack_mem, size_t stack_mem_size);

// switches to coroutine
// currently it's impossible to run one coroutine from another one
void koro_run(koro_t * h);

// yields from coroutine
void koro_yield(void);

#ifdef __cplusplus
}
#endif
