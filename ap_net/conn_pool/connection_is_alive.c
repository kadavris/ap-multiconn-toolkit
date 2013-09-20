#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_connection_is_alive()";
*/

/* ********************************************************************** */
/** \brief Returns AP_NET_ST_* for given connection state check
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \return int
 *
 * return bit field containing:
 * AP_NET_ST_ERROR if error occured in process or on connection itself
 * AP_NET_ST_AVAILABLE if connection is unused
 * AP_NET_ST_IN if data available for reading
 * AP_NET_ST_CONNECTED if also believed to be alive
 */
int ap_net_conn_pool_connection_is_alive(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct ap_net_connection_t *conn;
    int events;
    int status;


    conn = &pool->conns[conn_idx];

    if ( ! ( conn->state & AP_NET_ST_CONNECTED ) )
        return 0;

    if ( timerisset( &conn->expire ) && 0 <= ap_utils_timeval_cmp_to_now(&conn->expire) )
    {
       ap_net_conn_pool_close_connection(pool, conn_idx, NULL);
       return 0;
    }

    events = ap_net_poller_single_fd(conn->fd);

    if (events == -1)
        return AP_NET_ST_ERROR;

    status = AP_NET_ST_CONNECTED;

    if ( events & (EPOLLERR | EPOLLHUP ) ) /* connection's ERROR? */
        status |= AP_NET_ST_ERROR;

    if ( events & EPOLLIN ) /* data available to read */
        status |= AP_NET_ST_IN;

    return status;
}

