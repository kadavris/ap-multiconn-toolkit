/* \file ap_error/ap_error.h
 * \brief Part of AP's Toolkit. Error info storage module header
 * \internal
*/
#ifndef AP_ERROR_H

#include "../ap_log.h"

#ifndef AP_ERROR_C
extern void ap_error_set(const char *in_function_name, int in_errno);
extern void ap_error_set_detailed(const char *in_function_name, int in_errno, char *fmt, ...);
extern void ap_error_set_custom(const char *in_function_name, char *fmt, ...);
extern void ap_error_clear(void);
extern int ap_error_get(void);
extern const char *ap_error_get_string(void);
#endif

#endif
