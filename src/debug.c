#include <debug.h>

#ifdef KORO_SET_HARDWARE_BREAKPOINTS

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// this code is loosely based on https://github.com/mmorearty/hardware-breakpoints

void SetBits(uintptr_t * dw, uintptr_t lowBit, uintptr_t bits, uintptr_t newValue)
{
  // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111
  uintptr_t mask = (UINTMAX_C(1) << bits) - UINTMAX_C(1);
  *dw = ((*dw) & ~(mask << lowBit)) | (newValue << lowBit);
}

void _koro_set_hw_break_at(void * ptr)
{
  uint8_t when = 1; // w = 1, rw = 3
  uint8_t len = sizeof(uint32_t);

  switch(len)
  {
  case 1: len = 0; break;
  case 2: len = 1; break;
  case 4: len = 3; break;
  default: return;
  }

  CONTEXT ctx = {0};
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

  HANDLE t = GetCurrentThread();
  if(!GetThreadContext(t, &ctx))
    return;

  uintmax_t index = 0;
  bool set = false;
  for(uintmax_t i = 0; i < 4; ++i)
    if((ctx.Dr7 & (UINTMAX_C(1) << (i * UINTMAX_C(2)))) == 0)
    {
      index = i;
      set = true;
      break;
    }
  if(!set)
    return;

  switch(index)
  {
  case 0: ctx.Dr0 = (uintptr_t)ptr; break;
  case 1: ctx.Dr1 = (uintptr_t)ptr; break;
  case 2: ctx.Dr2 = (uintptr_t)ptr; break;
  case 3: ctx.Dr3 = (uintptr_t)ptr; break;
  default: return;
  }

  SetBits(&ctx.Dr7, 16 + (index * 4), 2, when);
  SetBits(&ctx.Dr7, 18 + (index * 4), 2, len);
  SetBits(&ctx.Dr7, index * 2,        1, 1);

  SetThreadContext(t, &ctx);
}

void _koro_clear_hw_break(void * ptr)
{
  CONTEXT ctx = {0};
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

  HANDLE t = GetCurrentThread();
  if(!GetThreadContext(t, &ctx))
    return;

  uintmax_t index = 0;
  if(ctx.Dr0 == (uintptr_t)ptr)
    index = 0;
  else if(ctx.Dr1 == (uintptr_t)ptr)
    index = 1;
  else if(ctx.Dr2 == (uintptr_t)ptr)
    index = 2;
  else if(ctx.Dr3 == (uintptr_t)ptr)
    index = 3;
  else
    return;

  SetBits(&ctx.Dr7, index * 2, 1, 0);

  SetThreadContext(t, &ctx);
}

#else

void _koro_set_hw_break_at(void * ptr) {}
void _koro_clear_hw_break(void * ptr) {}

#endif

#endif
