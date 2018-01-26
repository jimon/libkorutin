#pragma once

#include <libkorutin.h>

// for yield to work we need to know the handle of current running coroutine
// but it would be very annoying to require to pass 'h' to koro_yield
// as this will result in forwarding that handle everywhere (as we could yield in nested function calls)

// this helpers provide an easy (and thread aware way) to set/get currently running coroutine

void     _koro_set_current(koro_t * h);
koro_t * _koro_get_current();
