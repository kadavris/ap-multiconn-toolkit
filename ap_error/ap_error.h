#ifndef AP_ERROR_H
/* Part of AP's Toolkit
  ap_error/ap_error.h - internal!
  error info storage
*/

#include "../ap_log.h"
#include "internal_errnos.h"

#ifndef AP_ERROR_C
extern void ap_error_set(const char *in_function_name, int in_errno);
extern void ap_error_set_detailed(const char *in_function_name, int in_errno, char *fmt, ...);
extern void ap_error_set_custom(const char *in_function_name, char *fmt, ...);
extern void ap_error_clear(void);
extern int ap_error_get(void);
extern const char *ap_error_get_string(void);
#endif

#endif
