#ifndef AP_UTILS_H
#define AP_UTILS_H

#include <time.h>

// for ap_utils_timeval_set()
#define AP_UTILS_TIMEVAL_ADD 0
#define AP_UTILS_TIMEVAL_SET 1
#define AP_UTILS_TIMEVAL_SUB 2

#ifndef AP_UTILS_C
extern int ap_utils_timeval_set(struct timeval &tv, int mode, int msec);
#endif
#endif
