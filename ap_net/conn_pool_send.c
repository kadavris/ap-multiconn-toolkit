/** \file ap_net/conn_pool_send.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Outgoing data processing procedures
 */
#include "conn_pool_internals.h"
#include <errno.h>

static const char *_func_name = "ap_net_conn_pool_send()";

/* ********************************************************************** */
/** \brief send data _asynchronously_ from user's buffer
 *
 * \param pool struct ap_net_conn_pool_t *
 * \param conn_idx int
 * \param src_buf void* - source buffer with data
 * \param size int - length of data
 * \return int - actual amount sent
 *
 *  Tries to send as much data as possible in one run.
 */
int ap_net_conn_pool_send_async(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size)
{
    int n;
    int send_chunk;
    socklen_t slen;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( conn_idx < 0 || conn_idx > pool->max_connections || ! bit_is_set(conn->state, AP_NET_ST_CONNECTED) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_INVALID_CONN_INDEX, "%d", conn_idx);
        return 0;
    }

    conn->state |= AP_NET_ST_OUT;

	slen = bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

	send_chunk = size;

    for(;;)
    {
		if ( bit_is_set(pool->flags, AP_NET_POOL_FLAGS_TCP) )
			n = ap_net_send(conn->fd, src_buf, send_chunk, 1);
		else
			n = sendto(conn->fd, src_buf, send_chunk, 0, (struct sockaddr *)&conn->remote, slen);

		bit_clear(conn->state, AP_NET_ST_OUT);

		if ( n > 0 )
			break;

		if ( errno == EAGAIN || errno == EWOULDBLOCK )
		{
			if ( send_chunk < 10 ) /*  well, we tried */
				break;

			send_chunk /= 2;
			continue;
		}
		else if ( (n == -1 && errno == EPIPE) || n == 0)
		{
			if (ap_log_debug_level)
				ap_log_debug_log("? ap_net_conn_pool_send(): Connection #%d is dead prematurely: %m\n", conn_idx);

			ap_net_conn_pool_close_connection(pool, conn_idx);
			break;
		}
    }

    return n;

}
/* ********************************************************************** */
/** \brief send data from user's buffer with blocking
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \param src_buf void* - source buffer with data
 * \param size int - amount of data to send
 * \return int - actual amount sent or -1 on error
 *
 * This function send data synchronously with blocking if no AP_NET_POOL_FLAGS_ASYNC flag is set on pool
 * In other case the ap_net_conn_pool_send_async() called in place
 * If error detected on connection, then ap_net_conn_pool_close_connection() is called
 */
int ap_net_conn_pool_send(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size)
{
    int n;
    struct ap_net_connection_t *conn;


    if ( bit_is_set(pool->flags, AP_NET_POOL_FLAGS_ASYNC) )
    	return ap_net_conn_pool_send_async(pool, conn_idx, src_buf, size);

    ap_error_clear();

    conn = &pool->conns[conn_idx];

    if ( conn_idx < 0 || conn_idx > pool->max_connections || ! bit_is_set(conn->state, AP_NET_ST_CONNECTED) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_INVALID_CONN_INDEX, "%d", conn_idx);
        return 0;
    }

    conn->state |= AP_NET_ST_OUT;

    n = ap_net_send(conn->fd, src_buf, size, 0);

    bit_clear(conn->state, AP_NET_ST_OUT);

    if (n == -1 && errno == EPIPE)
    {
        if (ap_log_debug_level)
            ap_log_debug_log("? ap_net_conn_pool_send(): Connection #%d is dead prematurely: %m\n", conn_idx);

        ap_net_conn_pool_close_connection(pool, conn_idx);
    }

    return n;
}

