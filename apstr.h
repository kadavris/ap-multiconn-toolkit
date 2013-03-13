#ifndef _AP_STR_H
#define _AP_STR_H
#include <stdio.h>

#ifndef AP_STR_C
extern int debug_to_tty, debug_level;
#endif

extern int add_debug_handle(int fd); //register file/stream handle for debug output
extern int remove_debug_handle(int fd); //removes handle from debug output
extern int is_debug_handle(int fd); //checks if handle is registered for debug output
extern void debuglog(char *fmt, ...); //logs message to debug channel(s)
extern void dosyslog(int priority, char *fmt, ...); //syslog wrapper. also call debuglog if level is set

extern int hprintf(int fh, char *fmt, ...); //like fprintf for int handles
extern int hputs(char *str, int fh); //like fputs for int handles
extern int hputc(char c, int fh); //like fputc for int handles

extern void *getmem(int size, char *errmsg); //malloc with err checking
extern int makestr(char **d, char *s); // strdup with auto free/malloc d - destination ptr, s - source

// checks if designated buffer can hold the data expectedand resizes it if it is smaller than needed
extern int check_buf_size(char **buf, int *bufsize, int *bufpos, int needbytes);

// puts data into buffer, with size checking and buffer expanding on the fly
extern int put_to_buf(char **buf, int *bufsize, int *bufpos, void *src, int srclen);

extern void memdumpfd(int fh, void *p, int len); //hexdump/printable chars into specified filehandle
extern void memdump(void *p, int len); // hexdump/printable chars to debug channel(s)
extern void memdumpbfd(int fh, void *p, int len); // bitdump into specified filehandle
extern void memdumpb(void *p, int len); // bitdump into debug channel(s)
#endif
