#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_send()";

/* ********************************************************************** */
/** \brief send data from user's buffer
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \param src_buf void*
 * \param size int
 * \return int
 *
 */
int ap_net_conn_pool_send(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size)
{
    int n;
    socklen_t slen;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( conn_idx < 0 || conn_idx > pool->max_connections || ! (conn->state & AP_NET_ST_CONNECTED) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_INVALID_CONN_INDEX, "%d", conn_idx);
        return 0;
    }

    conn->state |= AP_NET_ST_OUT;

    switch ( pool->type )
    {
       case AP_NET_CTYPE_TCP:
           n = ap_net_tcpsend(conn->fd, src_buf, size);
           break;

      case AP_NET_CTYPE_UDP:
           slen = sizeof(struct sockaddr_in);
           n = sendto(conn->fd, src_buf, size, 0, (struct sockaddr *)&conn->addr, slen);
           break;
    }

    bit_clear(conn->state, AP_NET_ST_OUT);

    if ( (n == -1 && errno == EPIPE) || n == 0)
    {
        if (ap_log_debug_level)
            ap_log_debug_log("? ap_net_conn_pool_send(): Connection #%d is dead prematurely: %m\n", conn_idx);

        ap_net_conn_pool_close_connection(pool, conn_idx, NULL);
    }

    return n;
}

