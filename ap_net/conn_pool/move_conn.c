#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_move_conn()";
*/

/* ********************************************************************** */
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

    memcpy(dst_conn, src_conn, sizeof(struct ap_net_connection_t));
    memcpy(dst_conn->buf, src_conn->buf, dst_conn->bufsize > src_conn->bufsize ? src_conn->bufsize : dst_conn->bufsize);

    bit_clear(src_conn->state, AP_NET_ST_CONNECTED);
    src_conn->user_data = NULL;

    ap_net_conn_pool_poller_remove_conn(src_pool, conn_idx);

    ap_net_conn_pool_unlock(src_pool);

    dst_conn->parent = dst_pool;
    dst_conn->idx = dst_conn_idx;

    if ( dst_conn->bufptr > dst_conn->bufsize )
        dst_conn->bufptr = dst_conn->bufsize;

    if ( dst_conn->buffill > dst_conn->bufsize )
        dst_conn->buffill = dst_conn->bufsize;

    ap_net_conn_pool_poller_add_conn(dst_pool, dst_conn_idx);

    ap_net_conn_pool_unlock(dst_pool);

    return 1;
}
