#pragma once

#include <libkorutin.h>

void _koro_backend_init(koro_t * h);
void _koro_run(koro_t * h);
void _koro_yield(koro_t * h);
