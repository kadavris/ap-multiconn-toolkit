#ifndef AP_UTILS_C
#define AP_UTILS_C
#include "ap_utils.h"

int ap_utils_timeval_set(struct timeval &tv, int mode, int msec)
{
  struct timeval tmp;


  if ( msec < 100 )
    return 0;

  switch( mode )
  {
    case AP_UTILS_TIMEVAL_ADD:
      tmp.tv_sec = msec / 1000;
      tmp.tv_usec = 1000 * (msec % 1000);
      timeradd(tv, &tmp, tv);
      break;

    case AP_UTILS_TIMEVAL_SUB:
      tmp.tv_sec = msec / 1000;
      tmp.tv_usec = 1000 * (msec % 1000);
      timersub(tv, &tmp, tv);
      break;

    case AP_UTILS_TIMEVAL_SET:
      gettimeofday(tv, NULL);
      tmp.tv_sec = msec / 1000;
      tmp.tv_usec = 1000 * (msec % 1000);
      timeradd(tv, &tmp, tv);
      break;

    default:
      return 0;
  }

  return 1;
}

#endif
