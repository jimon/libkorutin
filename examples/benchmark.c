
#define COROUTINE_COUNT (16 * 1024)
#define STACK_SIZE 1024
#define CYCLES_PER_TICK_COUNT 100

#include <libkorutin.h>
#include <stdio.h>
#include <stdlib.h>
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
static void int_handler(int foo)
{
  (void)foo;
  run = 0;
}

int main()
{
  signal(SIGINT, int_handler);

  koro_t * k = malloc(sizeof(koro_t) * COROUTINE_COUNT);
  memset(k, 0, sizeof(koro_t) * COROUTINE_COUNT);

  co_ctx_t * ctx = malloc(sizeof(co_ctx_t) * COROUTINE_COUNT);
  memset(ctx, 0, sizeof(co_ctx_t) * COROUTINE_COUNT);

  for(size_t i = 0; i < COROUTINE_COUNT; ++i)
    koro_init(k + i, _co, ctx + i, malloc(STACK_SIZE), STACK_SIZE);

  size_t round = 0;

  while(run)
  {
    struct timeval before, after;
    gettimeofday(&before, NULL);

    // run a cycle
    size_t switches = 0;
    for(size_t c = 0; c < CYCLES_PER_TICK_COUNT; ++c)
    {
      round++;
      for(size_t i = 0; i < COROUTINE_COUNT; ++i)
      {
        koro_run(k + i);
        switches++;
      }
    }

    gettimeofday(&after, NULL);

    double elapsed = (after.tv_sec - before.tv_sec) + ((after.tv_usec - before.tv_usec) / 1000000.0);

    printf("%.1f switches/sec\n", (double)switches / elapsed); fflush(stdout);

    // check that all coroutines executed equal amount
    for(size_t i = 0; i < COROUTINE_COUNT; ++i)
      assert(ctx[i].i == round);
  }

  for(size_t i = 0; i < COROUTINE_COUNT; ++i)
    free(k[i].stack_mem);

  free(ctx);
  free(k);

  return 0;
}
