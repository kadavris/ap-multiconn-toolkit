#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"
#include <unistd.h>

static const char *_func_name = "ap_net_conn_pool_close_connection()";

/* ********************************************************************** */
/** \brief close connection by index, msg !=NULL to post some answer before close
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \param msg char* Last message to send to peer before closing (dangerous!)
 * \return void
 *
 */
void ap_net_conn_pool_close_connection(struct ap_net_conn_pool_t *pool, int conn_idx, char *msg)
{
    struct timeval tv;
    int used_as_debug_handle;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( ! (conn->state & AP_NET_ST_CONNECTED) )
        return;

    used_as_debug_handle = ap_log_is_debug_handle(conn->fd);

    if ( used_as_debug_handle )
        ap_log_remove_debug_handle(conn->fd);

    if ( msg != NULL && ! (conn->state & AP_NET_ST_ERROR) )
        ap_net_conn_pool_send(pool, conn_idx, msg, strlen(msg));

    ap_net_conn_pool_poller_remove_conn(pool, conn_idx);

    conn->state = 0;

    if (conn->type == AP_NET_CTYPE_TCP)
        close(conn->fd);

    conn->fd = 0;

    --pool->used_slots;

    if ( ! used_as_debug_handle ) /*  debug connections will not count for execution time */
    {
        gettimeofday(&tv, NULL);
        timersub(&tv, &conn->created_time, &tv);

        if ( conn->parent != NULL )
        	timeradd(&conn->parent->stat.total_time, &tv, &conn->parent->stat.total_time);
    }

    if (pool->close_func != NULL)
        pool->close_func(conn);

    if ( ap_log_debug_level > 0 )
        ap_log_debug_log("* %s: #[%d] closed\n", _func_name, conn_idx);
}

