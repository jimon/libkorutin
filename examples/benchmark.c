
#define SAMPLES_COUNT         32
#define STACK_SIZE            1024

#include <libkorutin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#ifdef _WIN32
// from https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct timeval {
  long tv_sec;
  long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
  // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
  // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
  // until 00:00:00 January 1, 1970
  static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

  SYSTEMTIME  system_time;
  FILETIME    file_time;
  uint64_t    time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time =  ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tp->tv_sec  = (long)((time - EPOCH) / 10000000L);
  tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
  return 0;
}
#else
#include <sys/time.h>
#endif

typedef struct
{
  uintptr_t i;
} co_ctx_t;

static void _co(co_ctx_t * ctx)
{
  while(true)
  {
    ctx->i++;
    koro_yield();
  }
}

static volatile int run = 1;

#ifndef KORO_BENCHMARK_NO_MAIN

static void int_handler(int foo)
{
  (void)foo;
  run = 0;
}

#endif

double benchmark(size_t coroutine_count, size_t cycles_per_tick_count)
{
  koro_t * k = malloc(sizeof(koro_t) * coroutine_count);
  memset(k, 0, sizeof(koro_t) * coroutine_count);

  co_ctx_t * ctx = malloc(sizeof(co_ctx_t) * coroutine_count);
  memset(ctx, 0, sizeof(co_ctx_t) * coroutine_count);

  for(size_t i = 0; i < coroutine_count; ++i)
    koro_init(k + i, (koro_func_t)_co, ctx + i, malloc(STACK_SIZE), STACK_SIZE);

  size_t round = 0;
  size_t count = 0;
  double samples[SAMPLES_COUNT] = {0};

  while(run && count < SAMPLES_COUNT)
  {
    struct timeval before, after;
    gettimeofday(&before, NULL);

    // run a cycle
    size_t switches = 0;
    for(size_t c = 0; c < cycles_per_tick_count; ++c)
    {
      round++;
      for(size_t i = 0; i < coroutine_count; ++i)
      {
        koro_run(k + i);
        switches++;
      }
    }

    gettimeofday(&after, NULL);

    double elapsed = (after.tv_sec - before.tv_sec) + ((after.tv_usec - before.tv_usec) / 1000000.0);
    double switches_per_sec = (double)switches / elapsed;
    samples[count++] = switches_per_sec;

    printf("%u:%u: %.1f switches/sec\n", (uint32_t)coroutine_count, (uint32_t)count, switches_per_sec); fflush(stdout);

    // check that all coroutines executed equal amount
    for(size_t i = 0; i < coroutine_count; ++i)
      assert(ctx[i].i == round);
  }

  for(size_t i = 0; i < coroutine_count; ++i)
    free(k[i].stack_mem);

  free(ctx);
  free(k);

  double avg = 0.0;
  for(size_t i = 0; i < SAMPLES_COUNT; ++i)
    avg += samples[i];
  avg /= (double)SAMPLES_COUNT;
  return avg;
}

#ifndef KORO_BENCHMARK_NO_MAIN

int main()
{
  signal(SIGINT, int_handler);

  #ifdef KORO_SET_HARDWARE_BREAKPOINTS
    printf("WARNING: KORO_SET_HARDWARE_BREAKPOINTS is enabled, which will decrease performance a lot!\n");
  #endif

  #ifdef KORO_BACKEND_SWITCH
    double perf_1024 = benchmark(1024, 1000);
    double perf_16384 = benchmark(16384, 50);

    printf("results for switch backend:\n");
    printf("%u: %.1f switches/sec\n", 1024, perf_1024); fflush(stdout);
    printf("%u: %.1f switches/sec\n", 16384, perf_16384); fflush(stdout);
  #endif

  #ifdef KORO_BACKEND_CONTEXT
    double perf_1024 = benchmark(1024, 1000);
    double perf_16384 = benchmark(16384, 50);

    printf("results for context backend:\n");
    printf("%u: %.1f switches/sec\n", 1024, perf_1024); fflush(stdout);
    printf("%u: %.1f switches/sec\n", 16384, perf_16384); fflush(stdout);
  #endif

  #ifdef KORO_BACKEND_THREADS
    double perf_32 = benchmark(32, 200);
    double perf_64 = benchmark(64, 50);
    double perf_128 = benchmark(128, 10);

    printf("results for threads backend:\n");
    printf("%u: %.1f switches/sec\n", 32, perf_32); fflush(stdout);
    printf("%u: %.1f switches/sec\n", 64, perf_64); fflush(stdout);
    printf("%u: %.1f switches/sec\n", 128, perf_128); fflush(stdout);
  #endif

  #ifdef KORO_BACKEND_EMSCRIPTEN
    double perf_1024 = benchmark(1024, 1000);
    double perf_16384 = benchmark(16384, 50);

    printf("results for emscripten backend:\n");
    printf("%u: %.1f switches/sec\n", 1024, perf_1024); fflush(stdout);
    printf("%u: %.1f switches/sec\n", 16384, perf_16384); fflush(stdout);
  #endif

  return 0;
}

#endif
