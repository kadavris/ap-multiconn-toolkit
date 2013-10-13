/* Part of AP's Toolkit
  ap_error/ap_error.c - internal!
  error processing module
*/
#define AP_ERROR_C

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ap_error.h"
#include "internal_strings.h"

extern int ap_log_get_lock(void);
extern void ap_log_release_lock(void);

#define ap_error_str_maxlen 1024

static int ap_log_err_errno; /* last error code. see ap_error.h for numbers */
static int ap_log_syserrno; /* copy of system errno value at the time of last error */
static char *ap_log_err_function_name; /* function name where error occurred. points to static strings inside modules */
static char ap_log_err_details[ap_error_str_maxlen + 1]; /* details about current error */

/* ********************************************************************** */
/** \brief INTERNAL. stores short info of error occurred in the toolkit's functions
 *
 * \param in_function_name char * - the function name or place of error
 * \param in_errno int - AP_ERRNO*
 * \return void
 *
 * INTERNAL. Stores basic error info. See ap_error/'*.h' for additional stuff
 */
void ap_error_set(const char *in_function_name, int in_errno)
{
    ap_log_syserrno = errno;
    ap_log_err_errno = in_errno;
    ap_log_err_function_name = (char *)in_function_name;
    *ap_log_err_details = '\0';
}

/* ********************************************************************** */
/** \brief INTERNAL. Stores detailed info of error occurred in the toolkit's functions
 *
 * \param in_function_name char * - the function name or place of error
 * \param in_errno int - AP_ERRNO*
 * \param fmt char* - printf() like
 * \return void
 *
 * INTERNAL. Stores extended error info. See ap_error/'*.h' for additional stuff
 * String generated by fmt will be truncated to ap_error_str_maxlen
 */
void ap_error_set_detailed(const char *in_function_name, int in_errno, char *fmt, ...) /*  internal. stores some message if error occured in the toolkit's functions */
{
    va_list vl;
    char *debug_msg;


    if ( ! ap_log_get_lock() )
        return;

    ap_log_syserrno = errno;
    ap_log_err_errno = in_errno;
    ap_log_err_function_name = (char *)in_function_name;

    va_start(vl, fmt);
    vsnprintf(ap_log_err_details, ap_error_str_maxlen, fmt, vl);
    va_end(vl);

    if (ap_log_debug_level)
    {
        debug_msg = (char *)ap_error_get_string();
        debuglog_output(debug_msg, strlen(debug_msg));
    }

    ap_log_release_lock();
}

/* ********************************************************************** */
/** \brief INTERNAL. Stores detailed info of error occurred in the toolkit's functions
 *
 * \param in_function_name char * - the function name or place of error
 * \param fmt char* - printf() like
 * \return void
 *
 * INTERNAL. Stores extended error info with AP_ERRNO_CUSTOM_MESSAGE error type. See ap_error/'*.h' for additional stuff
 * String generated by fmt will be truncated to ap_error_str_maxlen
 */
void ap_error_set_custom(const char *in_function_name, char *fmt, ...) /*  internal. stores some message if error occured in the toolkit's functions */
{
    va_list vl;


    va_start(vl, fmt);
    ap_error_set_detailed(in_function_name, AP_ERRNO_CUSTOM_MESSAGE, fmt, vl);
    va_end(vl);
}

/* ********************************************************************** */
/** \brief INTERNAL. Clears toolkit's error indicators. Must be called first within functions that may set some error info
 *
 * \return void
 *
 * INTERNAL. Clears error state of toolkit
 */
void ap_error_clear(void) /*  clears error message */
{
    ap_log_err_errno = ap_log_syserrno = 0;
    ap_log_err_function_name = "no name is set";
    *ap_log_err_details = '\0';
}

/* ********************************************************************** */
/** \brief Returns toolkit's error number.
 *
 * \return int
 *
 */
int ap_error_get(void)
{
    return ap_log_err_errno;
}

/* ********************************************************************** */
/** \brief Return string representation of last error occurred within toolkit's functions
 *
 * \param void
 * \return const char*
 *
 * Returned string is a pointer to the function's static buffer.
 * It is overwrited with current state data on any subsequent call to this function
 */
const char *ap_error_get_string(void)
{
    static char buf[ap_error_str_maxlen + 1]; /* output string */
    int bufpos;


    if ( ap_log_err_errno == 0 && ap_log_syserrno == 0 && errno == 0 )
    {
    	sprintf(buf, ap_log_errno_strings[ap_log_err_errno]);
    	return buf;
    }

    if ( ap_log_err_errno == 0 && errno != 0 )
    {
        snprintf(buf, ap_error_str_maxlen, "There is no error recorded inside AP's_toolkit functions, but system error is: %d/'%s'. ", errno, strerror(errno));
        return (const char *)buf;
    }

    bufpos = 0;

    bufpos += snprintf(buf + bufpos, ap_error_str_maxlen - bufpos, "ERROR in AP's_toolkit: in function: %s - ", ap_log_err_function_name);

    if ( ap_log_syserrno )
        bufpos += snprintf(buf + bufpos, ap_error_str_maxlen - bufpos, "system error: %d/'%s'. ", ap_log_syserrno, strerror(ap_log_syserrno));

	if ( *ap_log_err_details )
    	strncat(buf + bufpos, ap_log_err_details, ap_error_str_maxlen - bufpos);
    else if ( ! ap_log_syserrno )
    	bufpos += snprintf(buf + bufpos, ap_error_str_maxlen - bufpos, "%d/'%s'. %s",	ap_log_err_errno, ap_log_errno_strings[ap_log_err_errno], ap_log_err_details);

    return (const char *)buf;
}