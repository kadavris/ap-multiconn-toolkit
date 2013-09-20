#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_recv()";

/* ********************************************************************** */
/** \brief receives available data into internal buffer
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \return int
 *
 * this should be safe to call frm outside of this modeule,
 * but it's really internal thing
 */
int ap_net_conn_pool_recv(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    int n;
    socklen_t slen;
    int space_left;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( conn_idx < 0 || conn_idx > pool->max_connections || ! (conn->state & AP_NET_ST_CONNECTED) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_INVALID_CONN_INDEX, "%d", conn_idx);
        return -1;
    }

    if ( conn->bufptr >= conn->bufsize )
    {
        conn->bufptr = 0;
        conn->buffill = 0;
    }

    space_left = conn->bufsize - conn->buffill;

    if ( conn->bufptr > (conn->bufsize - conn->bufsize / 3) ) /*  take 2/3 fullness as signal to move data to beginning */
    {
       n = conn->buffill - conn->bufptr;
       memcpy(conn->buf, conn->buf + conn->bufptr, n);
       conn->buffill = n;
       conn->bufptr = 0;
       space_left = conn->bufsize - conn->buffill;
    }

    conn->state |= AP_NET_ST_IN;

    switch ( pool->type )
    {
       case AP_NET_CTYPE_TCP:
           n = ap_net_tcprecv(conn->fd, conn->buf + conn->bufptr, space_left);
           break;

       case AP_NET_CTYPE_UDP:
           slen = sizeof(struct sockaddr_in);
           n = recvfrom(conn->fd, conn->buf + conn->bufptr, space_left, MSG_DONTWAIT, (struct sockaddr *)&conn->addr, &slen );
           break;
    }

    if ( (n == -1 && errno == EPIPE) || n == 0)
    {
        if (ap_log_debug_level)
            ap_log_debug_log("? Connection [%d] is dead prematurely\n", conn_idx);

        ap_net_conn_pool_close_connection(pool, conn_idx, NULL);

        return -1;
    }

    bit_clear(conn->state, AP_NET_ST_IN);
    conn->buffill += n;

    return n;
}

