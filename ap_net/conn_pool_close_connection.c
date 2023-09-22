/** \file ap_net/conn_pool_close_connection.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: connection close procedures
 */
#include "conn_pool_internals.h"
#include <unistd.h>

static const char *_func_name = "ap_net_conn_pool_close_connection()";

/* ********************************************************************** */
/** \brief Close connection by index
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \return void
 *
 * Emits AP_NET_SIGNAL_CONN_CLOSING signal to pool's callback function.
 * Closes socket, marking connection available, updates statistics on pool
 */
void ap_net_conn_pool_close_connection(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct timespec ts;
    int used_as_debug_handle;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( ! (conn->state & AP_NET_ST_CONNECTED) )
        return;

    if (pool->callback_func != NULL)
        pool->callback_func(conn, AP_NET_SIGNAL_CONN_CLOSING);

    used_as_debug_handle = ap_log_is_debug_handle(conn->fd);

    if ( used_as_debug_handle )
        ap_log_remove_debug_handle(conn->fd);

    ap_net_conn_pool_poller_remove_conn(pool, conn_idx);

    conn->state = 0;

    close(conn->fd);

    conn->fd = -1;

    pool->used_slots--;

    if ( ! used_as_debug_handle && conn->parent != NULL ) /*  debug connections will not count for execution time */
    {
        ap_utils_timespec_elapsed(&conn->created_time, NULL, &ts);
        ap_utils_timespec_add(&conn->parent->stat.total_time, &ts, &conn->parent->stat.total_time);
    }

    if ( ap_log_debug_level > 0 )
        ap_log_debug_log("* %s: #%d closed\n", _func_name, conn_idx);
}

