/* Part of AP's Toolkit
 * ap_utils.h
 * Main include for ap_utils.c modules
 * Miscellaneous small tools to ease tedious tasks
 */

#ifndef AP_UTILS_H
#define AP_UTILS_H

#include <sys/time.h>
#include <stdint.h>

/* for ap_utils_timeval_set() adds to current value */
#define AP_UTILS_TIME_ADD 0
/* subtracts from current value */
#define AP_UTILS_TIME_SUB 1
/* set value to current time + offset */
#define AP_UTILS_TIME_SET_FROM_NOW 2
/* set value to offset only */
#define AP_UTILS_TIME_SET_FROMZERO 3

#ifndef AP_UTILS_C
extern int ap_utils_timeval_cmp_to_now(struct timeval *tv); /**< performs tv cmp now, returning -1 if less, 0 if == now and 1 if past current time */
extern int ap_utils_timeval_set(struct timeval *tv, int mode, int msec);
extern int ap_utils_timespec_cmp_to_now(struct timespec *ts);
extern void ap_utils_timespec_clear(struct timespec *ts);
extern int ap_utils_timespec_set(struct timespec *ts, int mode, int msec);
extern int ap_utils_timespec_is_set(struct timespec *ts);
extern void ap_utils_timespec_add(struct timespec *a, struct timespec *b, struct timespec *destination);
extern void ap_utils_timespec_sub(struct timespec *a, struct timespec *b, struct timespec *destination);
extern long ap_utils_timespec_elapsed(struct timespec *begin, struct timespec *end, struct timespec *destination);
extern long ap_utils_timespec_to_milliseconds(struct timespec *ts);
extern uint16_t count_crc16(void *mem, int len);
#endif

#define bit_get(p,m) ((p) & (m))
#define bit_is_set(p,m) (((p) & (m)) != 0)
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (1 << (x))

#endif
