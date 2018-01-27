#pragma once

#include <libkorutin.h>

#ifdef KORO_SET_HARDWARE_BREAKPOINTS
void _koro_set_hw_break_at(void * ptr);
void _koro_clear_hw_break(void * ptr);
#endif
