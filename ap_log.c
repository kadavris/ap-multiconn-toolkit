#define AP_LOG_C

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include "ap_error/ap_error.h"
#include "ap_error/internal_errnos.h"
#include "ap_error/internal_strings.h"
#include "ap_log.h"
#include "ap_net/ap_net.h"

int ap_log_debug_to_tty = 0;
int ap_log_debug_level = 0;

#define max_debug_handles 5
static int debug_handles[max_debug_handles];
static int debug_handles_count = 0;
static int debug_mutex = 0; /* lock flag */

#define ap_error_str_maxlen 1024

static int ap_log_errno; /* last error code. see ap_error.h for numbers */
static int ap_log_syserrno; /* copy of system errno value at the time of last error */
static char *ap_log_err_function_name; /* function name where error occurred. points to static strings inside modules */
static char ap_error_details[ap_error_str_maxlen]; /* details about current error */

/* ********************************************************************** */
static int get_lock(void)
{
    int i;


    for(i = 0; debug_mutex; ++i)
    {
        usleep(100000); /* 100ms */

        if (i == 3)
        {
            if ( ap_log_debug_to_tty )
                fputs("? WARNING! stale debug mutex!\n", stderr);

            return 0;
        }
    }

    ++debug_mutex;

    return 1;
}

/* ********************************************************************** */
static void release_lock(void)
{
    debug_mutex = 0;
}

/* ********************************************************************** */
static int is_debug_handle_internal(int fd)
{
    int i;


    for ( i = 0; i < debug_handles_count; ++i )
        if ( debug_handles[i] == fd )
            return 1;

    return 0;
}

/* ********************************************************************** */
/** \brief Adds opened tty or socket descriptor to the list of debugging channels
 *
 * \param fd int
 * \return int
 *
 * You can have debugging messages by debuglog() appear on any number
 * of file descriptors in your software.
 * So you can telnet to your application's binded port then issue some 'show debug' command
 * and instantly get all debug on screen.
 * In parallel you can open log file for writing and add it's handle to the debug pool.
 */
int ap_log_add_debug_handle(int fd)
{
    if ( !get_lock() )
        return 0;

    if ( is_debug_handle_internal(fd) )
    {
        release_lock();
        return 1;
    }

    if ( debug_handles_count == max_debug_handles )
    {
        release_lock();
        return 0;
    }

    debug_handles[debug_handles_count++] = fd;
    release_lock();

    return 1;
}

/* ********************************************************************** */
static int remove_debug_handle_internal(int fd)
{
    int i, ii;


    for ( i = 0; i < debug_handles_count; ++i )
    {
        if ( debug_handles[i] == fd )
        {
            for ( ii = i+1; ii < debug_handles_count; ++ii )
            debug_handles[ii - 1] = debug_handles[ii];

            --debug_handles_count;

            return 1;
        }
    }

    return 0;
}

/* ********************************************************************** */
/** \brief Counterpart to add_debug_handle(). Removes file descriptor from the debug output pool.
 *
 * \param fd int
 * \return int
 *
 */
int ap_log_remove_debug_handle(int fd)
{
    int retcode;


    if ( ! get_lock() )
        return 0;

    retcode = remove_debug_handle_internal(fd);
    release_lock();

    return retcode;
}

/* ********************************************************************** */
/** \brief Checking is this file descriptor is regitered for debug output
 *
 * \param fd int
 * \return int
 *
 */
int ap_log_is_debug_handle(int fd)
{
    int retcode;


    if ( ! get_lock() )
        return 0;

    retcode = is_debug_handle_internal(fd);
    release_lock();

    return retcode;
}

/* ********************************************************************** */
static void debuglog_output(char *buf, int buflen) /*  internal. outputs ready message to debug channel(s) */
{
    int i;


    if ( ap_log_debug_to_tty )
        fputs(buf, stderr);

    for ( i = 0; i < debug_handles_count; ++i )
    {
        int n = ap_net_poller_single_fd(debug_handles[i]);

        if ( n & (EPOLLERR | EPOLLHUP ) )
            remove_debug_handle_internal(debug_handles[i]);
        else
            write(debug_handles[i], buf, buflen); /* don't use tcpsend,etc, as it is may try to show some debug and do the loop */
    }
}

/* ********************************************************************** */
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
void ap_log_debug_log(char *fmt, ...)
{
    va_list vl;
    int buflen;
    char buf[1024];
    static int repeats = 0;    /* this statics is for syslog-like "last msg repeated N times..." */
    static char lastmsg[1024];
    static int lastlen = 0;
    static time_t last_time;


    if ( ! get_lock() )
        return;

    va_start(vl, fmt);
    buflen = vsnprintf(buf, 1023, fmt, vl);
    va_end(vl);

    if ( lastlen == buflen && 0 == strncmp(lastmsg, buf, lastlen) ) /*  repeat ? */
    {
        ++repeats;

        if ( last_time + 3 >= time(NULL) ) /*  3 sec delay between reposts */
        {
            buflen = sprintf(buf, "... Last message repeated %d time(s)\n", repeats);
            debuglog_output(buf, buflen);
            repeats = 0;
            last_time = time(NULL);

            release_lock();
            return;
        }
    }

    if ( repeats > 0 ) /*  should we repost final N repeats for previous message? */
    {
        lastlen = sprintf(lastmsg, "... and finally repeated %d time(s)\n", repeats);
        debuglog_output(lastmsg, lastlen);
    }

    strcpy(lastmsg, buf);
    lastlen = buflen;
    last_time = time(NULL);
    repeats = 0;

    debuglog_output(buf, buflen);

    release_lock();
}

/* ********************************************************************** */
/** \brief Sends your message to the syslog facility and to the debugging channel(s) if any
 *
 * \param priority int
 * \param fmt char*
 * \param ...
 * \return void
 *
 * Use it in place of plain syslog() call to get the non-debug information on the debug channels too
 */
void ap_log_do_syslog(int priority, char *fmt, ...)
{
    va_list vl;
    char buf[1024];


    va_start(vl, fmt);
    vsnprintf(buf, 1023, fmt, vl);
    va_end(vl);

    syslog(priority, buf);

    if ( ap_log_debug_level )
        ap_log_debug_log(buf);
}

/* ********************************************************************** */
void ap_error_set(const char *in_function_name, int in_errno) /*  internal. stores some message if error occured in the toolkit's functions */
{
    ap_log_syserrno = errno;
    ap_log_errno = in_errno;
    ap_log_err_function_name = (char *)in_function_name;
    *ap_error_details = '\0';
}

/* ********************************************************************** */
void ap_error_set_detailed(const char *in_function_name, int in_errno, char *fmt, ...) /*  internal. stores some message if error occured in the toolkit's functions */
{
    va_list vl;
    char *debug_msg;


    if ( ! get_lock() )
        return;

    ap_log_syserrno = errno;
    ap_log_errno = in_errno;
    ap_log_err_function_name = (char *)in_function_name;

    va_start(vl, fmt);
    vsnprintf(ap_error_details, ap_error_str_maxlen, fmt, vl);
    va_end(vl);

    if (ap_log_debug_level)
    {
        debug_msg = (char *)ap_error_get_string();
        debuglog_output(debug_msg, strlen(debug_msg));
    }

    release_lock();
}

/* ********************************************************************** */
void ap_error_set_custom(const char *in_function_name, char *fmt, ...) /*  internal. stores some message if error occured in the toolkit's functions */
{
    va_list vl;


    va_start(vl, fmt);
    ap_error_set_detailed(in_function_name, AP_ERRNO_CUSTOM_MESSAGE, fmt, vl);
    va_end(vl);
}

/* ********************************************************************** */
void ap_error_clear(void) /*  clears error message */
{
    ap_log_errno = 0;
    ap_log_err_function_name = "no name is set";
    *ap_error_details = '\0';
}

/* ********************************************************************** */
int ap_error_get(void)
{
    return ap_log_errno;
}

/* ********************************************************************** */
/** \brief Return string representation of last error occurred within toolkit's functions
 *
 * \param void
 * \return const char*
 *
 */
const char *ap_error_get_string(void)
{
    static char buf[ap_error_str_maxlen]; /* output string */
    int bufpos;


    bufpos = 0;

    if ( ap_log_syserrno )
        bufpos += snprintf(buf, ap_error_str_maxlen - 1, "system error: %d/'%s'. ",
                           ap_log_syserrno, strerror(ap_log_syserrno));

    bufpos += snprintf(buf + bufpos, ap_error_str_maxlen - bufpos - 1, "AP's_toolkit: in function: %s - ", ap_log_err_function_name);

	if ( ap_log_errno == AP_ERRNO_CUSTOM_MESSAGE )
    	strncat(buf + bufpos, ap_error_details, ap_error_str_maxlen - bufpos - 1);
    else
    	snprintf(buf + bufpos, ap_error_str_maxlen - bufpos - 1, "%d/'%s'. %s",	ap_log_errno, ap_log_errno_strings[ap_log_errno], ap_error_details);

    return (const char *)buf;
}

/* ********************************************************************** */
/** \brief fprintf() for standard file handles. Can printf() to socket
 *
 * \param file_handle int
 * \param fmt char*
 * \param ...
 * \return int
 *
 */
int ap_log_hprintf(int file_handle, char *fmt, ...)
{
    va_list vl;
    int blen,retcode;
    char buf[1024];


    va_start(vl, fmt);

    blen = vsnprintf(buf, 1024, fmt, vl);
    retcode = write(file_handle, buf, blen);

    va_end(vl);

    return retcode;
}

/* ********************************************************************** */
/** \brief fputs for standard filehandles. simplifies writing plain string to sockets
 *
 * \param str char*
 * \param file_handle int
 * \return int
 *
 */
int ap_log_hputs(char *str, int file_handle)
{
    return write(file_handle, str, strlen(str));
}

/* ********************************************************************** */
/** \brief fputc for standard filehandles. just to be looking uniform with tty output
 *
 * \param c char
 * \param file_handle int
 * \return int
 *
 */
int ap_log_hputc(char c, int file_handle)
{
    return write(file_handle, &c, 1);
}

/* ********************************************************************** */
/** \brief memory hex dump with printable characters shown
 *
 * \param file_handle int
 * \param memory_area void*
 * \param len int
 * \return void
 *
 * prints pretty standard dump like
 * 0x0000: 01 02 03 64 75 6D 70     . . . d u m p
 * to the opened file or socket
 */
void ap_log_mem_dump_to_fd(int file_handle, void *memory_area, int len)
{
    int i;
    int addr;
    int showaddr;
    int linelen;
    unsigned char *s;


    if (len == 0)
        return;

    showaddr = (len > 48);/*  > 3 lines of data */
    addr = 0;

    s = (unsigned char*)memory_area;

    for (; len > 0; len -= 16, addr += 16)
    {
        if (showaddr)
            ap_log_hprintf(file_handle, "%04x(%4d):  ", addr, addr);

        linelen = len > 16 ? 16 : len;

        for (i = 0; i < linelen; ++i)
            ap_log_hprintf(file_handle, "%02x %c ", s[i], isprint(s[i]) ? s[i] : '.');

        /*
        for(i = 0; i < linelen; ++i) hprintf(file_handle, "%02x ", s[i]);
        for(i = 16 - linelen; i > 0; --i) hputs("   ", file_handle);

        hprintf(file_handle, "\n\t");
        for(i = 0; i < linelen; ++i) hprintf(file_handle, " %c ", isprint(s[i]) ? s[i] : '.');
        */
        ap_log_hputc('\n', file_handle);

        s += linelen;
    }
}

/* ********************************************************************** */
/** \brief memory hex dump with printable characters shown. output to debug channel(s) if any or to stderr
 *
 * \param memory_area void*
 * \param len int
 * \return void
 *
 * prints pretty standard dump like
 * 0x0000: 01 02 03 64 75 6D 70     . . . d u m p
 * to the debug channel(s)
 */
void ap_log_mem_dump(void *memory_area, int len)
{
    int i;


    if (ap_log_debug_to_tty)
        ap_log_mem_dump_to_fd(fileno(stderr), memory_area, len);

    for ( i = 0; i < debug_handles_count; ++i )
        if (debug_handles[i] != 0)
        ap_log_mem_dump_to_fd(debug_handles[i], memory_area, len);
}

/* ********************************************************************** */
/** \brief memory area bits dump direct to file or socket descriptor
 *
 * \param file_handle int
 * \param memory_area void*
 * \param len int
 * \return void
 *
 * prints pretty standard dump like
 * 0x00: 01001011
 * to the opened file or socket
 */
void ap_log_mem_dump_bits_to_fd(int file_handle, void *memory_area, int len)
{
    int i;
    int mask;
    int size;
    unsigned char *s;


    if (len == 0)
        return;

    s = (unsigned char*)memory_area;

    for (; len > 0; len -= 4)
    {
        ap_log_hputs("\t", file_handle);

        size = len > 4 ? 4 : len;

        for(i = 0; i < size; ++i)
        {
            ap_log_hprintf(file_handle, "0x%02x: ", s[i]);

            for (mask = 128; mask != 0; mask >>= 1)
                ap_log_hputc( s[i] & mask ? '1' : '0', file_handle );

            ap_log_hputs("   ", file_handle);
        }

        ap_log_hputs("\n", file_handle);
    }
}

/* ********************************************************************** */
/** \brief memory area bits dump to debugging channel(s)
 *
 * \param memory_area void*
 * \param len int
 * \return void
 *
 * prints pretty standard dump like
 * 0x00: 01001011
 * to all debugging channel(s) or stderr
 */
void ap_log_mem_dump_bits(void *memory_area, int len)
{
    int i;


    if (ap_log_debug_to_tty)
        ap_log_mem_dump_bits_to_fd(fileno(stderr), memory_area, len);

    for ( i = 0; i < debug_handles_count; ++i )
        if (debug_handles[i] != 0)
            ap_log_mem_dump_bits_to_fd(debug_handles[i], memory_area, len);
}

