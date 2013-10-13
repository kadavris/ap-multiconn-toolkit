/* Part of AP's Toolkit
 * Logging and debugging module
 * ap_log.c
 */
#define AP_LOG_C

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ap_error/ap_error.h"
#include "ap_log.h"
#include "ap_net/ap_net.h"

int ap_log_debug_to_tty = 0; /**< If set to true will output ap_log_debug_log() messages also to stderr */
int ap_log_debug_level = 0;  /**< Global debugging messages verbosity level */

#define max_debug_handles 5 /**< Maximum count of file/socket handles that can be registered for simultaneous debug output */
static int debug_handles[max_debug_handles];
static int debug_handles_count = 0; /* Current number of registered handles */
static int debug_mutex = 0; /* lock flag for internal functions */

/* **********************************************************************
 * Internal
 */
int ap_log_get_lock(void)
{
    int i;


    for(i = 0; debug_mutex; ++i)
    {
        usleep(100000); /* 100ms */

        if (i == 3)
        {
            if ( ap_log_debug_to_tty )
                fputs("? ap_log: WARNING! stale debug mutex!\n", stderr);

            return 0;
        }
    }

    ++debug_mutex;

    return 1;
}

/* **********************************************************************
 * Internal
 */
void ap_log_release_lock(void)
{
    debug_mutex = 0;
}

/* **********************************************************************
 * Checking if supplied file handle is already registered
 */
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
 * \return int - true/false
 *
 * You can have debugging messages by ap_log_debug_log() appear on any number
 * of file descriptors in your software.
 * So you can telnet to your application's binded port then issue some 'show debug' command
 * and instantly get all debug on screen.
 * In parallel you can open log file for writing and add it's handle to the debug pool.
 */
int ap_log_add_debug_handle(int fd)
{
    if ( !ap_log_get_lock() )
        return 0;

    if ( is_debug_handle_internal(fd) )
    {
        ap_log_release_lock();
        return 1;
    }

    if ( debug_handles_count == max_debug_handles )
    {
        ap_log_release_lock();
        return 0;
    }

    debug_handles[debug_handles_count++] = fd;
    ap_log_release_lock();

    return 1;
}

/* **********************************************************************
 * removes provided file handle from list of debugging channels
 */
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
/** \brief Counterpart to ap_log_add_debug_handle(). Removes file descriptor from the debug output pool.
 *
 * \param fd int
 * \return int - true/false
 *
 * You can have debugging messages by ap_log_debug_log() appear on any number of opened file or socket descriptors.
 * So you can telnet to your application's binded port then issue some 'show debug' command
 * and instantly get all debug on screen.
 * In parallel you can open log file for writing and add it's handle to the debug pool.
 */
int ap_log_remove_debug_handle(int fd)
{
    int retcode;


    if ( ! ap_log_get_lock() )
        return 0;

    retcode = remove_debug_handle_internal(fd);
    ap_log_release_lock();

    return retcode;
}

/* ********************************************************************** */
/** \brief Checking is this file descriptor is already registered for debug output
 *
 * \param fd int - opened file or socket handle
 * \return int - true/false
 *
 */
int ap_log_is_debug_handle(int fd)
{
    int retcode;


    if ( ! ap_log_get_lock() )
        return 0;

    retcode = is_debug_handle_internal(fd);
    ap_log_release_lock();

    return retcode;
}

/* **********************************************************************
 * Internal. outputs ready message to debug channel(s)
 */
void debuglog_output(char *buf, int buflen)
{
    int i;


    if ( ap_log_debug_to_tty )
        fputs(buf, stderr);

    for ( i = 0; i < debug_handles_count; ++i )
    {
    	if ( S_ISREG(debug_handles[i]) ) /* regular file? */
    	{
            write(debug_handles[i], buf, buflen);
            continue;
    	}

    	/* socket */
		/*
        n = ap_net_poller_single_fd(debug_handles[i]);
        if ( n & AP_NET_POLLER_ST_ERROR )
        {
            remove_debug_handle_internal(debug_handles[i]);
            i--;
        }
        */

		if ( 0 >= send(debug_handles[i], buf, buflen, MSG_NOSIGNAL))
		{
            remove_debug_handle_internal(debug_handles[i]);
            i--;
		}
    }
}

/** \brief Outputs messages to the stderr if set so, and to the all of registered debugging channels at once
 *
 * \param fmt char* Format string like in printf()
 * \param ... Optional parameters
 * \return void
 *
 * Use with if ( debug_level > some_number ) ap_log_debug_log(message_format_string, some, args, may, follow);
 * First you need to register opened log file descriptor or remote connected socket
 * as debug handle(s) by calling ap_log_add_debug_handle(fd). Release handle by ap_log_release_debug_handle(fd)
 * stderr output is triggered by setting the global variable ap_log_debug_to_tty to true
 */
void ap_log_debug_log(char *fmt, ...)
{
    va_list vl;
    int buflen;
    char buf[1024];
    static int repeats = 0;    /* those statics is for syslog-like "last msg repeated N times..." */
    static char lastmsg[1024];
    static int lastlen = 0;
    static time_t last_time;


    if ( ! ap_log_get_lock() )
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

            ap_log_release_lock();
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

    ap_log_release_lock();
}

/* ********************************************************************** */
/** \brief Sends your message to the syslog facility and to the debugging channel(s) if any
 *
 * \param priority int syslog() priority flags
 * \param fmt char* - printf() - like format string
 * \param ... Optional parameters
 * \return void
 *
 * Use it in place of plain syslog() call to get the debugging and/or logging information on the debug channels too
 * Output to the debug channel(s) is triggered when global variable ap_log_debug_level > 0
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
/** \brief fprintf() for standard file handles. Can printf() to socket
 *
 * \param file_handle int - file or socket handle
 * \param fmt char* - printf() alike
 * \param ... Optional parameters
 * \return int - return code from system write() call
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

    if ( retcode <= 0 )
    	ap_error_set("ap_log_hprintf()", AP_ERRNO_SYSTEM);

    return retcode;
}

/* ********************************************************************** */
/** \brief fputs for standard filehandles. simplifies writing plain string to sockets
 *
 * \param str char* - output string
 * \param file_handle int  - file or socket handle
 * \return int - return code from system write() call
 *
 */
int ap_log_hputs(char *str, int file_handle)
{
    int retcode;


    retcode = write(file_handle, str, strlen(str));

    if ( retcode <= 0 )
    	ap_error_set("ap_log_hprintf()", AP_ERRNO_SYSTEM);

    return retcode;
}

/* ********************************************************************** */
/** \brief fputc for standard filehandles. just to be looking uniform with tty output
 *
 * \param c char - Output char
 * \param file_handle int - file or socket handle
 * \return int - return code from system write() call
 *
 */
int ap_log_hputc(char c, int file_handle)
{
    return write(file_handle, &c, 1);
}

/* ********************************************************************** */
/** \brief Memory hex dump with printable characters shown
 *
 * \param file_handle int - output file or socket handle
 * \param memory_area void* - origin of data
 * \param len int - length of data to dump
 * \return void
 *
 * prints pretty standard dump like
 * 0x0000: 01 . 02 . 03 . 64 d 75 u 6D m 70 p
 * to the opened file or socket
 * You may use it like ap_log_mem_dump_to_fd(fileno(stderr), ...) to dump to stderr, etc.
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
/** \brief Memory hex dump with printable characters shown. output to debug channel(s) if any or to stderr
 *
 * \param memory_area void* - What to dump
 * \param len int - length of data
 * \return void
 *
 * prints pretty standard dump like
 * 0x0000: 01 . 02 . 03 . 64 d 75 u 6D m 70 p
 * to the debug channel(s), ignoring ap_log_debug_level
 * Set global variable ap_log_debug_to_tty to true for output to stderr too.
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
/** \brief Memory area bits dump direct to file or socket descriptor
 *
 * \param file_handle int - output file or socket handle
 * \param memory_area void* - What to dump
 * \param len int - length of data
 * \return void
 *
 * prints pretty standard dump like
 * 0x00: 01001011
 * to the opened file or socket
 * You may use it like ap_log_mem_dump_bits_to_fd(fileno(stderr), ...) to dump to stderr, etc.
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
 * \param memory_area void* - What to dump
 * \param len int - length of data
 * \return void
 *
 * prints pretty standard dump like
 * 0x00: 01001011
 * to the debug channel(s), ignoring ap_log_debug_level
 * Set global variable ap_log_debug_to_tty to true for output to stderr too.
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

