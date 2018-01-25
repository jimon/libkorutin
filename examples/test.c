
#include <libkorutin.h>

#include <stdio.h>
#include <stdlib.h>

static int * foo = NULL;

void test_co1(void * ctx)
{
  int a = 12345;
  foo = &a; // stack data can be shared between coroutines
  for(int i = 0; i < 20; ++i)
  {
    printf("%s:%u %p %i %i\n", __func__, __LINE__, &i, i, *foo); fflush(stdout);
    koro_yield();
  }
}

void test_co2(void * ctx)
{
  for(int i = 0; i < 10; ++i)
  {
    printf("%s:%u %p %i %i\n", __func__, __LINE__, &i, i, *foo); fflush(stdout);
    koro_yield();
  }
}

static void test_co3_helper2(void)
{
  koro_yield();
}

static void test_co3_helper1(void)
{
  test_co3_helper2();
}

void test_co3(void * ctx)
{
  for(int i = 0; i < 5; ++i)
  {
    printf("%s:%u %p %i %i\n", __func__, __LINE__, &i, i, *foo); fflush(stdout);
    test_co3_helper1();
  }
}

void test_co4(void * ctx)
{
  printf("%s:%u\n", __func__, __LINE__); fflush(stdout);
  // coroutines can exit just fine
}

int main()
{
  koro_t k1, k2, k3, k4;

  // printf is a stack hungry beast, so far my ballpark measurements:
  // - MacOS   needs ~2kb of stack for printf call
  // - Windows needs ~4kb of stack for printf call
  uint8_t stack1[4 * 1024];
  uint8_t stack2[4 * 1024];
  uint8_t stack3[4 * 1024];
  uint8_t stack4[4 * 1024];

  koro_init(&k1, test_co1, NULL, stack1, sizeof(stack1));
  koro_init(&k2, test_co2, NULL, stack2, sizeof(stack2));
  koro_init(&k3, test_co3, NULL, stack3, sizeof(stack3));
  koro_init(&k4, test_co4, NULL, stack4, sizeof(stack4));

  for(size_t i = 0; i < 25; ++i)
  {
    printf("%s:%u %u %u %u %u\n", __func__, __LINE__,
      (uint32_t)koro_calculate_stack_watermark(&k1),
      (uint32_t)koro_calculate_stack_watermark(&k2),
      (uint32_t)koro_calculate_stack_watermark(&k3),
      (uint32_t)koro_calculate_stack_watermark(&k4)
    ); fflush(stdout);
    koro_run(&k1);
    koro_run(&k2);
    koro_run(&k3);
    koro_run(&k4);
  }
  return 0;
}
