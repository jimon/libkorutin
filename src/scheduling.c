#include "scheduling.h"

// -------------------------------------------------------------------------------------- thread aware storage

#if KORO_THREAD_AWARE

/*
_Thread_local is part of tinycthreads

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

      1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

      2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.

      3. This notice may not be removed or altered from any source
      distribution.
*/

  #if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
    #if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
      #define _koro_Thread_local __thread
    #else
      #define _koro_Thread_local __declspec(thread)
    #endif
  #elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
    #define _koro_Thread_local __thread
  #endif

#else
  #define _koro_Thread_local
#endif

// --------------------------------------------------------------------------------------

_koro_Thread_local static koro_t * current = NULL;

void _koro_set_current(koro_t * h)
{
  current = h;
}

koro_t * _koro_get_current()
{
  return current;
}
