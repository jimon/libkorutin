#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------------------------------- config

// define as 1 if you plan to use coroutines from multiple threads
// (you still can only run one coroutine at a time per thread)
#define KORO_THREAD_AWARE 0

// must be at least 16 on x64 platforms to store xmm registers
#define KORO_STACK_ALIGNMENT 16

// enable or disable watermarking, this fills whole stack with 0xfee1dead value
// and later use it to figure out peak stack usage (watermark)
#define KORO_WATERMARKING

// -------------------------------------------------------------------------------------- types

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
  #if KORO_BACKEND_SWITCH
  void * _stack_org;
  void * _stack_run;
  #endif
  #if KORO_BACKEND_THREADS
  void * _thrd;
  #endif
} koro_t;

// -------------------------------------------------------------------------------------- root functions

// initialise context structure
void koro_init(koro_t * h, koro_func_t fn, void * ctx, uint8_t * stack_mem, size_t stack_mem_size);

// switches to coroutine
// currently it's impossible to run one coroutine from another one
void koro_run(koro_t * h);

// yields from coroutine
void koro_yield(void);

// -------------------------------------------------------------------------------------- stack usage inspection

#ifdef KORO_WATERMARKING
// Warning! resource heavy operation, O(N) dependency from stack usage
// this function returns value rounded up to 4 bytes
size_t koro_calculate_stack_watermark(koro_t * h);
#endif

#ifdef __cplusplus
}
#endif
