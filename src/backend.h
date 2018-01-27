#pragma once

#include <libkorutin.h>

void _koro_backend_init(koro_t * h);
void _koro_run(koro_t * h);
void _koro_yield(koro_t * h);

// not all backends support externaly provided stack
#if defined(KORO_BACKEND_SWITCH) || defined(KORO_BACKEND_CONTEXT)
  #define KORO_EXTERNAL_STACK_AWARE
#endif
