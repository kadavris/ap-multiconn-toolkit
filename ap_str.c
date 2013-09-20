/*
  apstr.c: string manipulation and alike functions. written by Andrej Pakhutin for his own use primarily.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>

#define _AP_STR_C
#include "ap_log.h"
#include "ap_str.h"

/* ********************************************************************** */
/** \brief strdup() with automatic free/malloc
 *
 * \param d char**
 * \param s const char*
 * \return int
 *
 */
int ap_str_makestr(char **d, const char *s)
{
    if ( *d != NULL )
        free(*d);

    *d = NULL;

    if ( s == NULL )
        return 0;

    if ( NULL == (*d = (char*)malloc(strlen(s) + 1) ) )
        return 0;

    strcpy( (char*)(*d), s );

    return 1;
}

/* ********************************************************************** */
/** \brief malloc() helper with automatic self-destruction on out of memory
 *
 * \param size int
 * \param errmsg char*
 * \return void*
 *
 * Tries to allocate memory. if malloc() fails it outputs message from errmsg argument
 * to syslog facility and aborts program
 */
void *ap_str_getmem(int size, char *errmsg)
{
    void *ptr;


    ptr = malloc(size);

    if ( ptr == NULL )
    {
        if (errmsg != NULL)
            ap_log_do_syslog(LOG_ERR, errmsg);

        exit(1);
    }

    return ptr;
}

/* ********************************************************************** */
/** \brief dst_buf is checked for enough free space and then resized if being too small
 *
 * \param dst_buf char**
 * \param dst_buf_size int*
 * \param dst_buf_pos int*
 * \param need_bytes int
 * \return int
 *
 * Useful helper for standard trios of buffer/buffer_pos/buffer_size
 * Call it before copying or receiving known amount of data into buffer
 * to enlarge it if it needed. updates buffer_size variable as well
 */
int ap_str_fix_buf_size(char **dst_buf, int *dst_buf_size, int *dst_buf_pos, int need_bytes)
{
    int n;


    if ( *dst_buf_size - *dst_buf_pos < need_bytes )
    {
        n = *dst_buf_size + need_bytes - (*dst_buf_size - *dst_buf_pos);

        if ( NULL == realloc(*dst_buf, n) )
            return 0;

        *dst_buf_size = n;
    }

    return 1;
}

/* ********************************************************************** */
/** \brief copies data from src_buf to dst_buf. adjusting dst_buf's size and position marker
 *
 * \param dst_buf char**
 * \param dst_buf_size int*
 * \param dst_buf_pos int*
 * \param src_buf void*
 * \param src_len int
 * \return int
 *
 * dst_buf is checked for being smaller than needed by calling check_buf_size()
 * after resizing the data from src_buf copied and dst_buf_fill updated
 */
int ap_str_put_to_buf(char **dst_buf, int *dst_buf_size, int *dst_buf_fill, void *src_buf, int src_len)
{
    if ( ! ap_str_fix_buf_size(dst_buf, dst_buf_size, dst_buf_fill, src_len) )
        return 0;

    memcpy(*dst_buf + *dst_buf_fill, src_buf, src_len);

    *dst_buf_fill += src_len;

    return 1;
}

/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
/** \brief initializes parsing record using in_str as source
 *
 * \param in_str char*
 * \param user_separators char*
 * \return t_str_parse_rec*
 *
 * in_str is copied into parse record struct.
 * You can supply your own string of chars to be used as separators in parsing.
 * if it is NULL then defaul space and tab wil be used
 */
ap_str_parse_rec_t *ap_str_parse_init(char *in_str, char *user_separators)
{
    ap_str_parse_rec_t *r;


    if( NULL == (r = malloc(sizeof(ap_str_parse_rec_t))) )
        return NULL;

    r->separators = NULL;

    if ( ! ap_str_makestr(&r->separators, ( user_separators != NULL ) ? user_separators : " \t" ) )
    {
        free(r);
        return NULL;
    }

    r->buf_len = strlen(in_str) + 1;

    if( NULL == (r->buf = malloc(r->buf_len)) )
    {
        free(r);
        return NULL;
    }

    strcpy(r->buf, in_str);
    r->buf_pos = r->buf;

    r->curr = strsep(&r->buf_pos, r->separators);

    return r;
}

/* ********************************************************************** */
/** \brief Frees internal structures of parsing record and release it
 *
 * \param r t_str_parse_rec*
 * \return void
 *
 */
void ap_str_parse_end(ap_str_parse_rec_t *r)
{
    free(r->buf);
    free(r->separators);
    free(r);
}

/* ********************************************************************** */
/** \brief Returns next token from parsed string
 *
 * \param r t_str_parse_rec*
 * \return char*
 *
 */
char *ap_str_parse_next_arg(ap_str_parse_rec_t *r)
{
    return (r->curr = strsep(&r->buf_pos, r->separators));
}

/* ********************************************************************** */
/** \brief Returns leftover string w/o parsing
 *
 * \param r t_str_parse_rec*
 * \return char*
 *
 */
char *ap_str_parse_get_remaining(ap_str_parse_rec_t *r)
{
    return r->buf_pos;
}

/* ********************************************************************** */
/** \brief Returning roll_count tokens back to reparse again
 *
 * \param r t_str_parse_rec*
 * \param roll_count int
 * \return char*
 *
 */
char *ap_str_parse_rollback(ap_str_parse_rec_t *r, int roll_count)
{
    if ( roll_count <= 0 )
        return r->curr;

    for(; roll_count; --roll_count)
    {
        if( r->buf_pos == r->buf )
            break;

        if ( *(r->buf_pos) ) /*  checking if we're not at the end of parsed string */
            *(r->buf_pos - 1) = *(r->separators); /* replacing '\0' with first separator char */

        for(; r->buf_pos > r->buf; r->buf_pos--)
        {
            if( *r->buf_pos == '\0' ) /*  reached end of token */
            {
                r->buf_pos++;

                /*  backed up one token - looping for next if more pending */
                break;
            }
        }
    }

    return ap_str_parse_next_arg(r);
}

/* ********************************************************************** */
/** \brief Skips skip_count tokens forward.
 *
 * \param r t_str_parse_rec*
 * \param skip_count int
 * \return char*
 *
 */
char *ap_str_parse_skip(ap_str_parse_rec_t *r, int skip_count)
{
    char *s;


    while( skip_count-- && NULL != (s = ap_str_parse_next_arg(r)) );

    return s;
}

/* ********************************************************************** */
/** \brief Set separators array to the new value. space and tab defaults will be used if NULL
 *
 * \param r t_str_parse_rec*
 * \param separators char*
 * \return int
 *
 */
int str_parse_set_separators(ap_str_parse_rec_t *r, char *separators)
{
    if ( ! ap_str_makestr(&(r->separators), ( separators != NULL ) ? separators : " \t" ) )
        return 0;

    return 1;
}

#define GET_BOOL_KW_COUNT 8
static struct
{
    const char *s;
    const int result;
} keywords[GET_BOOL_KW_COUNT] = {
    { "1", 1 },
    { "0", 0 },
    { "on", 1 },
    { "off", 0 },
    { "true", 1 },
    { "false", 0 },
    { "enable", 1 },
    { "disable", 0 }
};

/* ********************************************************************** */
/** \brief Returns int 0 or 1 as a boolean value of next token. -1 if unrecognized
 *
 * \param r t_str_parse_rec*
 * \return int
 *
 * understands the following pairs:
 * 1,0
 * on,off
 * true,false
 * enable,disable
 */
int ap_str_parse_get_bool(ap_str_parse_rec_t *r)
{
    char *s;
    int i;


    s = ap_str_parse_next_arg(r);

    if ( s == NULL )
        return -1;

    for( i = 0; i < GET_BOOL_KW_COUNT; ++i )
        if ( 0 == strcasecmp(keywords[i].s, s ) )
            return keywords[i].result;

    return -1;
}
