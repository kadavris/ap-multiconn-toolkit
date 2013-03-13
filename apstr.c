/*
  apstr.c: string manipulation and alike functions. written by Andrej Pakhutin for his own use primarily.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>

#define _AP_STR_C
#include "apstr.h"
#include "aptcp.h"

int debug_to_tty = 0;
int debug_level = 0;

#define max_debug_handles 5
static int debug_handles[max_debug_handles];
static int debug_handles_count = 0;
static int debug_mutex = 0;

//=======================================================================
static int getlock(void)
{
  int i;

  for(i = 0; debug_mutex; ++i)
  {
    usleep(100000);//100ms

    if (i == 3)
    {
      if ( debug_to_tty )
        fputs("? WARNING! stale debug mutex!\n", stderr);

      return 0;
    }
  }

  ++debug_mutex;

  return 1;
}

//=================================================================
int is_debug_handle_internal(int fd)
{
  int i;

  for ( i = 0; i < debug_handles_count; ++i )
    if ( debug_handles[i] == fd )
      return 1;

  return 0;
}

//=================================================================
int add_debug_handle(int fd)
{
  if ( !getlock() )
    return 0;

  if ( is_debug_handle_internal(fd) )
  {
    debug_mutex = 0;
    return 1;
  }

  if ( debug_handles_count == max_debug_handles )
  {
    debug_mutex = 0;
    return 0;
  }

  debug_handles[debug_handles_count++] = fd;
  debug_mutex = 0;

  return 1;
}

//=================================================================
static int remove_debug_handle_internal(int fd)
{
  int i, ii;

  for ( i = 0; i < debug_handles_count; ++i )
  {
    if ( debug_handles[i] == fd )
    {
      for ( ii = i+1; ii < debug_handles_count; ++ii )
        debug_handles[ii-1] = debug_handles[ii];

      --debug_handles_count;

      return 1;
    }
  }
  return 0;
}

//=================================================================
int remove_debug_handle(int fd)
{
  int retcode;

  if ( !getlock() )
    return 0;

  retcode = remove_debug_handle_internal(fd);
  debug_mutex = 0;

  return retcode;
}

//=================================================================
int is_debug_handle(int fd)
{
  int retcode;

  if ( !getlock() )
    return 0;

  retcode = is_debug_handle_internal(fd);
  debug_mutex = 0;

  return retcode;
}

//=================================================================
void debuglog(char *fmt, ...)
{
  va_list vl;
  int blen,i,n;
  char buf[1024];

  if ( ! getlock() )
    return;

  va_start(vl, fmt);

  blen = vsnprintf(buf, 1024, fmt, vl);

  if ( debug_to_tty )
    fputs(buf, stderr);

  for ( i = 0; i < debug_handles_count; ++i )
  {
    n = tcp_check_state(debug_handles[i]);

    if ( n == -1 || (n & 4) != 0 )
      remove_debug_handle_internal(debug_handles[i]);
    else
      write(debug_handles[i], buf, blen); //don't use tcpsend,etc, as it is may try to show some debug and do the loop
  }

  debug_mutex = 0;
  va_end(vl);
}

//=================================================================
void dosyslog(int priority, char *fmt, ...)
{
  va_list vl;
  char buf[1024];

  va_start(vl, fmt);
  vsnprintf(buf, 1023, fmt, vl);

  syslog(priority, buf);

  if ( debug_level )
    debuglog(buf);

  va_end(vl);
}

//==========================================================
// fprintf for standard filehandles
int hprintf(int fh, char *fmt, ...)
{
  va_list vl;
  int blen,retcode;
  char buf[1024];

  va_start(vl, fmt);

  blen = vsnprintf(buf, 1024, fmt, vl);
  retcode = write(fh, buf, blen);

  va_end(vl);
  return retcode;
}

// fputs for standard filehandles
int hputs(char *str, int fh)
{
  return write(fh, str, strlen(str));
}

// fputc for standard filehandles
int hputc(char c, int fh)
{
  return write(fh, &c, 1);
}

//=================================================================
void *getmem(int size, char *errmsg)
{
  void *ptr;

  ptr = malloc(size);

  if ( ptr == NULL )
  {
    if (errmsg != NULL)
      dosyslog(LOG_ERR, errmsg);

    exit(1);
  }

  return ptr;
}

//=================================================================
/*
  strdup with auto free/malloc d - destination ptr, s - source
  should we simplify this via realloc?
*/
int makestr(char **d, char *s)
{
  if ( *d != NULL )
    free(*d);

  if ( s == NULL )
  {
    *d = NULL;
    return 0;
  }

  if ( (*d = (char*)malloc(strlen((char*)s) + 1) ) == NULL)
    return 0;

  strcpy( (char*)(*d), (char*)s );

  return 1;
}

//==========================================================
// memory hex dump with printable characters shown
void memdumpfd(int fh, void *p, int len)
{
int i, linelen, addr, showaddr;
unsigned char *s;

  if (len == 0)
     return;

  showaddr = (len > 48);// > 3 lines of data
  addr = 0;

  s = (unsigned char*)p;

  for (; len > 0; len -= 16, addr += 16)
  {
    if (showaddr)
      hprintf(fh, "%04x(%4d):  ", addr, addr);

    linelen = len > 16 ? 16 : len;

    for (i = 0; i < linelen; ++i)
      hprintf(fh, "%02x %c ", s[i], isprint(s[i]) ? s[i] : '.');

    /*
    for(i = 0; i < linelen; ++i) hprintf(fh, "%02x ", s[i]);
    for(i = 16 - linelen; i > 0; --i) hputs("   ", fh);

    hprintf(fh, "\n\t");
    for(i = 0; i < linelen; ++i) hprintf(fh, " %c ", isprint(s[i]) ? s[i] : '.');
    */
    hputc('\n', fh);
    s += linelen;
  }
}

// dumps to debug channel if any
void memdump(void *p, int len)
{
  int i;

  if (debug_to_tty)
    memdumpfd(fileno(stderr), p, len);

  for ( i = 0; i < debug_handles_count; ++i )
    if (debug_handles[i] != 0)
      memdumpfd(debug_handles[i], p, len);
}

//==========================================================
// memory bits dump
void memdumpbfd(int fh, void *p, int len)
{
int i, mask, size;
unsigned char *s;

  if (len == 0)
    return;

  s = (unsigned char*)p;

  for (; len > 0; len -= 4)
  {
    hputs("\t", fh);
    size = len > 4 ? 4 : len;

    for(i = 0; i < size; ++i)
    {
      hprintf(fh, "0x%02x: ", s[i]);

      for (mask = 128; mask != 0; mask >>= 1)
        hputc( s[i] & mask ? '1' : '0', fh );

      hputs("   ", fh);
    }
    hputs("\n", fh);
  }
}

// dumps to debug channel if any
void memdumpb(void *p, int len)
{
  int i;

  if (debug_to_tty)
    memdumpbfd(fileno(stderr), p, len);

  for ( i = 0; i < debug_handles_count; ++i )
    if (debug_handles[i] != 0)
      memdumpbfd(debug_handles[i], p, len);
}

//==========================================================
// copies data from src to buf. increasing size of the buf if it is smaller than needed
int check_buf_size(char **buf, int *bufsize, int *bufpos, int needbytes)
{
  int n;

  if ( *bufsize - *bufpos < needbytes )
  {
    n = *bufsize + needbytes - (*bufsize - *bufpos);

    if ( NULL == realloc(*buf, n) )
      return 0;

    *bufsize = n;
  }

  return 1;
}

//==========================================================
// copies data from src to buf. calling check_buf_size to adjust if needed
int put_to_buf(char **buf, int *bufsize, int *bufpos, void *src, int srclen)
{
  if ( ! check_buf_size(buf, bufsize, bufpos, srclen) )
    return 0;

  memcpy(*buf + *bufpos, src, srclen);
  *bufpos += srclen;

  return 1;
}
