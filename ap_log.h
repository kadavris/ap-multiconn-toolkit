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

/** \brief Outputs messages to the stderr if set so, and to the all of registered debugging channels at once
 *
 * \param fmt char*
 * \param ...
 * \return void
 *
 * Use with if ( debug_level > some_number ) debuglog(message_format_string, some, args, may , foolow);
 * First you need to register opened log file descriptor or remote connected socket
 * as debug handle(s) by calling add_debug_handle(fd). Release hande by release_debug_handle(fd)
 */
extern void ap_log_debug_log(char *fmt, ...);

extern void ap_log_do_syslog(int priority, char *fmt, ...); /**< syslog wrapper. also call debuglog if level is set */

extern const char *ap_error_get_string(void); /**<  returns string with info on last error occured within ap_* function calls */

/* ************************************* */
extern int ap_log_hprintf(int fh, char *fmt, ...); /**< like fprintf for int handles */
extern int ap_log_hputs(char *str, int fh); /**< like fputs for int handles */
extern int ap_log_hputc(char c, int fh); /**< like fputc for int handles */

/* ************************************* */
extern void ap_log_mem_dump_to_fd(int fh, void *p, int len); /**< hexdump/printable chars into specified filehandle */
extern void ap_log_mem_dump(void *p, int len); /**< hexdump/printable chars to debug channel(s) */
extern void ap_log_mem_dump_bits_to_fd(int fh, void *p, int len); /**< bitdump into specified filehandle */
extern void ap_log_mem_dump_bits(void *p, int len); /**< bitdump into debug channel(s) */


#endif
