/* Part of AP's Toolkit
 * Networking module
 * ap_net/conn_pool_move_conn.c
 */
#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_move_conn()";
*/

/* ********************************************************************** */
/** \brief receives available data into internal buffer
 *
 * \param dst_conn struct ap_net_connection_t* - destination connection structure pointer
 * \param src_conn struct ap_net_connection_t* - source connection structure pointer
 * \return void
 *
 * Making a partial copy of source structure into destination, making them carry same data, state,
 * but retain pool parents and other top bindings.
 * This is really for a preparation for moving connection from pool to pool
 * or between the slots in one pool before lowering connections amount
 */
void ap_net_connection_copy(struct ap_net_connection_t *dst_conn, struct ap_net_connection_t *src_conn)
{
	void *tmp;


    dst_conn->fd = src_conn->fd;
    memcpy(&dst_conn->remote, &src_conn->remote, sizeof(src_conn->remote));
    memcpy(&dst_conn->local, &src_conn->local, sizeof(src_conn->local));
    memcpy(&dst_conn->created_time, &src_conn->created_time, sizeof(src_conn->created_time));
    memcpy(&dst_conn->expire, &src_conn->expire, sizeof(src_conn->expire));

    memcpy(dst_conn->buf, src_conn->buf, dst_conn->bufsize > src_conn->bufsize ? src_conn->bufsize : dst_conn->bufsize);
    dst_conn->bufpos = src_conn->bufpos;
    dst_conn->buffill = src_conn->buffill;

    if ( dst_conn->bufpos > dst_conn->bufsize )
        dst_conn->bufpos = dst_conn->bufsize;

    if ( dst_conn->buffill > dst_conn->bufsize )
        dst_conn->buffill = dst_conn->bufsize;

    dst_conn->state = src_conn->state;

    tmp = dst_conn->user_data;
    dst_conn->user_data = src_conn->user_data;
    dst_conn->user_data = tmp;
}

/* ********************************************************************** */
/** \brief receives available data into internal buffer
 *
 * \param dst_pool struct ap_net_conn_pool_t* - destination pool pointer
 * \param src_pool struct ap_net_conn_pool_t* - source pool pointer
 * \param conn_idx int - Connection index in source pool
 * \return int - True on success, false on failure
 *
 * Moving connection from source pool to the destination pool' free slot.
 * Can be used on single pool to defrag connections list.
 * Used in ap_net_conn_pool_set_max_connections() before lowering max number of connections
 * If destination pool connections list if filled up, then ap_net_conn_pool_set_max_connections() called to enlarge the list by plus one.
 * So if you plan to move more than one connection issue ap_net_conn_pool_set_max_connections() manually with larger increment
 */
int ap_net_conn_pool_move_conn(struct ap_net_conn_pool_t *dst_pool, struct ap_net_conn_pool_t *src_pool, int conn_idx)
{
    int dst_conn_idx;
    struct ap_net_connection_t *src_conn;
    struct ap_net_connection_t *dst_conn;


    ap_error_clear();

    if ( ! ap_net_conn_pool_lock(dst_pool) )
       return 0;

    if ( dst_pool->used_slots == dst_pool->max_connections
         && ! ap_net_conn_pool_set_max_connections(dst_pool, dst_pool->max_connections + 1, dst_pool->conns[0].bufsize))
    {
        ap_net_conn_pool_unlock(dst_pool);
        /*ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "can't add more slots to destination pool");*/
        return 0;
    }

    if ( ! ap_net_conn_pool_lock(src_pool) )
    {
        ap_net_conn_pool_unlock(dst_pool);
        return 0;
    }

    for (dst_conn_idx = 0; dst_conn_idx < dst_pool->max_connections; ++dst_conn_idx) /*  finding free slot */
        if ( ! (dst_pool->conns[dst_conn_idx].state & AP_NET_ST_CONNECTED) )
            break;

    src_conn = &src_pool->conns[conn_idx];
    dst_conn = &dst_pool->conns[dst_conn_idx];

    ap_net_connection_copy(dst_conn, src_conn);

    ap_net_conn_pool_poller_remove_conn(src_pool, conn_idx);

    src_conn->fd = -1;

    bit_clear(src_conn->state, AP_NET_ST_CONNECTED);

    if ( src_pool->callback_func != NULL ) /* force reinit of user's data */
    	src_pool->callback_func(src_conn, AP_NET_SIGNAL_CONN_MOVED_FROM);

    ap_net_conn_pool_unlock(src_pool);

    ap_net_conn_pool_poller_add_conn(dst_pool, dst_conn_idx);

    if ( dst_pool->callback_func != NULL ) /* force reinit of user's data */
    	dst_pool->callback_func(dst_conn, AP_NET_SIGNAL_CONN_MOVED_TO);

    ap_net_conn_pool_unlock(dst_pool);

    return 1;
}
