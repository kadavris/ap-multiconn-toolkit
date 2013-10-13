/* Part of AP's Toolkit
 * Logging and debugging module
 * ap_log.h
 */
#ifndef AP_LOG_H
#define AP_LOG_H

#include <syslog.h>


#ifndef AP_LOG_C
extern int ap_log_debug_to_tty; /**< boolean flag telling whether to output debug messages to stderr also */
extern int ap_log_debug_level; /**< verboseness of debug messages. use  if ( debug_level >= MINLEVEL ) say_something; */
#endif

extern int ap_log_add_debug_handle(int fd); /**< register file/stream handle for debug output */

extern int ap_log_remove_debug_handle(int fd); /**< removes handle from debug output */

extern int ap_log_is_debug_handle(int fd); /**< checks if handle is registered for debug output */

extern void ap_log_debug_log(char *fmt, ...);
extern void debuglog_output(char *buf, int buflen);

extern void ap_log_do_syslog(int priority, char *fmt, ...); /**< syslog wrapper. also call debuglog if level is set */

extern int ap_error_get(void); /**< returns toolkit's error number for the last error occurred within ap_* function calls. 0 = no error */
extern const char *ap_error_get_string(void); /**< returns string with info on last error occurred within ap_* function calls */

/* ************************************* */
extern int ap_log_hprintf(int fh, char *fmt, ...); /**< like fprintf for int handles */
extern int ap_log_hputs(char *str, int fh); /**< like fputs for int handles */
extern int ap_log_hputc(char c, int fh); /**< like fputc for int handles */

/* ************************************* */
extern void ap_log_mem_dump_to_fd(int fh, void *p, int len); /**< hexdump/printable chars into specified file handle */
extern void ap_log_mem_dump(void *p, int len); /**< hexdump/printable chars to debug channel(s) */
extern void ap_log_mem_dump_bits_to_fd(int fh, void *p, int len); /**< bitdump into specified file handle */
extern void ap_log_mem_dump_bits(void *p, int len); /**< bitdump into debug channel(s) */


#endif
