#ifndef AP_ERROR_H
/*
  ap_error.h - internal!
  error info storage
*/

#include "../ap_log.h"
#include "internal_errnos.h"

#ifndef AP_LOG_C
extern void ap_error_set(const char *in_function_name, int in_errno);
extern void ap_error_set_detailed(const char *in_function_name, int in_errno, char *fmt, ...);
extern void ap_error_set_custom(const char *in_function_name, char *fmt, ...);
extern void ap_error_clear(void);
#endif

#endif
