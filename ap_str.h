/** \file ap_str.h
 * \brief Part of AP's Toolkit. Miscellaneous strings functions module. Main header
 */
#ifndef AP_STR_H
#define AP_STR_H

#include <stdio.h>

/** \brief strtok() wrapper functions data storage
*/
typedef struct ap_str_parse_rec
{
  char *buf, *buf_pos;
  int buf_len;
  char *curr;
  char *separators;
} ap_str_parse_rec_t;


extern void *ap_str_getmem(int size, char *errmsg); /**< malloc with err checking. if errmsg != NULL it is reported via dosyslog and exit(1) called */
extern int ap_str_makestr(char **d, const char *s); /**< strdup with auto free/malloc d - destination ptr, s - source */


/* checks if designated buffer can hold the data expectedand resizes it if it is smaller than needed */
extern int ap_str_fix_buf_size(char **buf, int *bufsize, int *bufpos, int needbytes);
/* puts data into buffer, with size checking and buffer expanding on the fly */
extern int ap_str_put_to_buf(char **buf, int *bufsize, int *bufpos, void *src, int srclen);


/* creates new parsing record from in_str, using user_separators if not NULL. default is space + tab */
extern ap_str_parse_rec_t *ap_str_parse_init(char *in_str, char *user_separators);
extern void ap_str_parse_end(ap_str_parse_rec_t *r); /**< destroys parse record */
extern char *ap_str_parse_next_arg(ap_str_parse_rec_t *r);/**< get next token from string */
extern char *ap_str_parse_get_remaining(ap_str_parse_rec_t *r); /**<  get remnants of string */
extern char *ap_str_parse_rollback(ap_str_parse_rec_t *r, int roll_count); /**<  rollback n tokens */
extern char *ap_str_parse_skip(ap_str_parse_rec_t *r, int skip_count); /**< skip n tokens forward */
/* get next arg as boolean value (on/off 1/0 true/false). -1 in case of error */
extern int ap_str_parse_get_bool(ap_str_parse_rec_t *r);

#endif
