/** \file ap_net/conn_pool_connection_is_alive.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Single connection state check procedures
 */
#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_connection_is_alive()";
*/

/* ********************************************************************** */
/** \brief Returns AP_NET_ST_* as a result of given connection state check
 *
 * \param pool struct ap_net_conn_pool_t * - pointer to pool structure
 * \param conn_idx int - Connection index in pool
 * \return int - bitfiled of AP_NET_ST_*
 *
 * return bit field containing:
 * AP_NET_ST_ERROR if error occurred in process or on connection itself
 * AP_NET_ST_AVAILABLE if connection is unused
 * AP_NET_ST_IN if data available for reading
 * AP_NET_ST_OUT if socket is ready for sending data to peer
 * AP_NET_ST_CONNECTED if also believed to be alive
 *
 * Closes connection on expire (conn->expire > 0)
 */
int ap_net_conn_pool_connection_is_alive(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct ap_net_connection_t *conn;
    int events;
    int status;


    conn = &pool->conns[conn_idx];

    if ( ! ( conn->state & AP_NET_ST_CONNECTED ) )
        return 0;

    if ( ap_utils_timespec_is_set( &conn->expire ) && 0 <= ap_utils_timespec_cmp_to_now(&conn->expire) )
    {
       ap_net_conn_pool_close_connection(pool, conn_idx);
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

    if ( events & EPOLLOUT ) /* can send data */
        status |= AP_NET_ST_OUT;

    return status;
}

