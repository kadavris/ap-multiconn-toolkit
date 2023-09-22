/** \file ap_net/conn_pool_accept_connection.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: accept connection procedures
 */
#include "conn_pool_internals.h"
#include <unistd.h>
#include <fcntl.h>

static const char *_func_name = "ap_net_conn_pool_accept_connection()";

/* ********************************************************************** */
/** \brief Accepts new connection on listened port and adds it to the pool's list
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return struct ap_net_connection_t *
 *
 * returns NULL on error. connection pointer if OK.
 *
 * TCP pool just do standard accept procedure, but UDP pool peeks for datagram's remote address and port
 * and if it is not in the pool already, creates outgoing connection to this address/port with pool's default expiration time
 * also signal AP_NET_SIGNAL_CONN_DATA_IN emitted on this connection afterwards
 */
struct ap_net_connection_t *ap_net_conn_pool_accept_connection(struct ap_net_conn_pool_t *pool)
{
    int n;
    socklen_t addr_len;
    int new_sock;
    struct ap_net_connection_t *conn;
    struct sockaddr_storage addr;
    struct sockaddr *remote_addr;


    ap_error_clear();

    if ( bit_is_set(pool->flags, AP_NET_POOL_FLAGS_TCP) )
    {
        conn = ap_net_conn_pool_find_free_slot(pool);

        if ( conn == NULL )
            return NULL;

        ap_net_conn_pool_connection_pre_connect(pool, conn->idx, 0);

        conn->remote.af = bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) ? AF_INET6 : AF_INET;
        remote_addr = bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) ? (struct sockaddr *)&conn->remote.addr6 : (struct sockaddr *)&conn->remote.addr4;
        addr_len = bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

        new_sock = accept(pool->listener.sock, remote_addr, &addr_len);

        if ( new_sock == -1 )
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "accept()");
            ap_net_connection_unlock(conn);
            return NULL;
        }

        fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL) | O_NONBLOCK);

        conn->fd = new_sock;

        if ( ! ap_net_conn_pool_poller_add_conn(pool, conn->idx) ) /* UDP adding new socket fd when do pool_connect(), so we need it here once */
        {
            ap_net_conn_pool_close_connection(pool, conn->idx);
            return NULL;
        }

        if (pool->callback_func != NULL && ! pool->callback_func(conn, AP_NET_SIGNAL_CONN_ACCEPTED) ) /* user disagreed */
        {
            ap_net_conn_pool_close_connection(pool, conn->idx);
            ap_error_set(_func_name, AP_ERRNO_ACCEPT_DENIED);
            return NULL;
        }

    }
    else /* UDP pool */
    {
        /* first peeking at the top of messages queue for just address of remote peer
         * then do connect from our side to lock remote address to the new connection's socket
         */
        addr_len = sizeof(addr);
        n = recvfrom(pool->listener.sock, &n, 1, MSG_DONTWAIT | MSG_PEEK, (struct sockaddr *)&addr, &addr_len);

        if ( n == -1 )
        {
            ap_error_set(_func_name, AP_ERRNO_SYSTEM);
            return NULL;
        }

        conn = ap_net_conn_pool_get_conn_by_address(pool, &addr, 0);

        if ( conn == NULL )
        {
            /* new connection */
            if ( addr.ss_family == AF_INET )
            {
                conn = ap_net_conn_pool_connect_ip4(pool, AP_NET_CONN_FLAGS_UDP_IN,
                            ntohl(((struct sockaddr_in*)&addr)->sin_addr.s_addr), ntohs(((struct sockaddr_in*)&addr)->sin_port),
                            ap_utils_timespec_to_milliseconds(&pool->max_conn_ttl));
            }
            else
            {
                conn = ap_net_conn_pool_connect_ip6(pool, AP_NET_CONN_FLAGS_UDP_IN,
                            &((struct sockaddr_in6*)&addr)->sin6_addr, ntohs(((struct sockaddr_in6*)&addr)->sin6_port),
                            ap_utils_timespec_to_milliseconds(&pool->max_conn_ttl));
            }

            if( conn == NULL )
                return NULL;

            if (pool->callback_func != NULL && ! pool->callback_func(conn, AP_NET_SIGNAL_CONN_ACCEPTED) ) /* user disagreed */
            {
                ap_net_conn_pool_close_connection(pool, conn->idx);
                ap_error_set(_func_name, AP_ERRNO_ACCEPT_DENIED);
                return NULL;
            }
        }

        n = ap_net_conn_pool_recv(pool, conn->idx);

        if ( n > 0 && pool->callback_func != NULL )
                pool->callback_func(conn, AP_NET_SIGNAL_CONN_DATA_IN);

        return conn;
    }

    if (ap_log_debug_level)
        ap_log_debug_log("* Got connected at #%d\n", conn->idx);

    ap_net_connection_unlock(conn);

    return conn;
}

