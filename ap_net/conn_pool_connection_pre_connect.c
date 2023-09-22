/** \file ap_net/conn_pool_connection_pre_connect.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Connection structure initialization procedures
 */
#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_connection_pre_connect()";
*/

/* ********************************************************************** */
/** \brief Fills connection's structure with default, sane data
 *
 * \param pool struct ap_net_conn_pool_t * - pool
 * \param conn_idx int - connection index in pool
 * \param flags unsigned - bit set of AP_NET_CONN_FLAGS_*
 * \return void
 *
 * This function called from inside toolkit's accept_connection() and connect()
 * to fill new connection record with default data.
 * It's not meant to be calling manually.
 */
void ap_net_conn_pool_connection_pre_connect(struct ap_net_conn_pool_t *pool, int conn_idx, unsigned flags)
{
    struct ap_net_connection_t *conn;


    conn = &pool->conns[conn_idx];

    ap_net_connection_lock(conn);

    ap_utils_timespec_set(&conn->created_time, AP_UTILS_TIME_SET_FROM_NOW, 0);

    if ( ap_utils_timespec_is_set( &pool->max_conn_ttl) )
    {
        ap_utils_timespec_add(&conn->created_time, &pool->max_conn_ttl, &conn->expire);
    }
    else
    {
        ap_utils_timespec_clear( &conn->expire );
    }

    conn->fd = -1;
    conn->flags = flags;
    conn->bufpos = 0;
    conn->buffill = 0;

    conn->state = AP_NET_ST_CONNECTED;

    pool->used_slots++;
    pool->stat.conn_count++;
    pool->stat.active_conn_count += pool->used_slots;
}
