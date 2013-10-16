/** \file ap_str.c
 * \brief Part of AP's Toolkit. Miscellaneous strings manupulation functions module.
 */
#define AP_STR_C
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>

#include "ap_log.h"
#include "ap_str.h"

/* ********************************************************************** */
/** \brief strdup() with automatic free/malloc
 *
 * \param d char** - Destination. pointer to variable holding the current string
 * \param s const char* - Source string
 * \return int - true/false
 *
 * Example:
 * // We set value to default first and if we get some other data we overwrite the default with new.
 * char *default = "Default value"; // that we will still have if n other data will arrive in buffer
 * char *buffer; // some input buffer
 * char *value = NULL; // note the initial value should be NULL. no static initializers allowed too, as it will be attempted to be free()'s
 *
 * if ( ! ap_str_makestr(&value, default)) // setting it to default value
 * 		error()
 *
 * if ( get_data_into_buffer(buffer) )
 * 		if ( ! ap_str_makestr(&value, buffer)) // and re-setting it to new value
 * 			error()
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
 * \param size int - How much to allocate
 * \param errmsg char* - string goes to syslog in case of error
 * \return void* - pointer to the newly allocated buffer.
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
 * \param dst_buf char** - pointer to receiver buffer variable
 * \param dst_buf_size int* - pointer to variable holding the size of receiver allocated pool
 * \param dst_buf_pos int* - ponter to variable holding the current position inside the receiver pool
 * \param need_bytes int - How much more data we need to put into receiver pool
 * \return int true if OK, false if malloc() failed
 *
 * Useful helper for standard trios of buffer/buffer_pos/buffer_size
 * Call it before copying or receiving known amount of data into buffer
 * to enlarge it if it needed. updates dst_buf_size variable as well
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
/** \brief Copies data from src_buf to dst_buf. adjusting dst_buf's size and position marker
 *
 * \param dst_buf char** - pointer to receiver buffer variable
 * \param dst_buf_size int* - pointer to variable holding the size of receiver allocated pool
 * \param dst_buf_fill int* - pointer to variable holding the position/offset of free space ant the end of inside the receiver pool
 * \param src_buf void* - source buffer
 * \param src_len int - length of data to be copied from source buffer
 * \return int
 *
 * dst_buf is checked by calling ap_str_fix_buf_size(), so if it's left space is smaller than needed,
 * the buffer is resized to provide more space.
 * after the check, the data from src_buf copied past dst_buf_fill point and dst_buf_fill updated
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
/** \brief Initializes parsing record using in_str as source. strsep() companion set
 *
 * \param in_str char* - source string to be parsed
 * \param user_separators char* - list of one-character separators for strsep()
 * \return t_str_parse_rec* - pointer to newly created parsing record
 *
 * in_str is copied into parse record struct.
 * You can supply your own string of chars to be used as separators in parsing.
 * if it is NULL then default 'space' and 'tab' will be used
 *
 * Call ap_str_parse_end() to free resources in the end.
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
/** \brief Frees internal structures of parsing record and release it. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
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
/** \brief Returns next token from parsed string. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
 * \return char* - token or NULL if end of string
 *
 */
char *ap_str_parse_next_arg(ap_str_parse_rec_t *r)
{
    return (r->curr = strsep(&r->buf_pos, r->separators));
}

/* ********************************************************************** */
/** \brief Returns leftover string w/o parsing. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
 * \return char* - string
 *
 */
char *ap_str_parse_get_remaining(ap_str_parse_rec_t *r)
{
    return r->buf_pos;
}

/* ********************************************************************** */
/** \brief Returning roll_count tokens back to reparse again. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
 * \param roll_count int - how many token to count back
 * \return char* - token or NULL if end of string
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
/** \brief Skips skip_count tokens forward. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
 * \param skip_count int - how many tokens to skip
 * \return char* - token or NULL if end of string
 *
 */
char *ap_str_parse_skip(ap_str_parse_rec_t *r, int skip_count)
{
    char *s;


    while( skip_count-- && NULL != (s = ap_str_parse_next_arg(r)) );

    return s;
}

/* ********************************************************************** */
/** \brief Set separators array to the new value. space and tab defaults will be used if NULL. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
 * \param separators char* - new list of separators
 * \return int - true/false
 *
 */
int str_parse_set_separators(ap_str_parse_rec_t *r, char *separators)
{
    return ap_str_makestr(&(r->separators), ( separators != NULL ) ? separators : " \t" );
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
/** \brief Returns int 0 or 1 as a boolean value of next token. -1 if unrecognized. strsep() companion set
 *
 * \param r t_str_parse_rec* - pointer to the parse structure created by ap_str_parse_init()
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
