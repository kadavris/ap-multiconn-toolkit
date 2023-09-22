/** \file ap_net/conn_pool_connect.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: connection to remote peer procedures
 */
#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"
#include "../ap_utils.h"
#include <unistd.h>
#include <fcntl.h>

static const char *_func_name = "ap_net_conn_pool_connect()";

static struct ap_net_connection_t *ap_net_conn_pool_do_connect(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct ap_net_connection_t *conn;
    socklen_t slen;


    conn = &pool->conns[conn_idx];

    conn->fd = socket( conn->remote.af, bit_is_set(pool->flags, AP_NET_POOL_FLAGS_TCP) ? SOCK_STREAM : SOCK_DGRAM, 0 );

    if ( -1 == conn->fd )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "socket()");
        ap_net_connection_unlock(conn);

        return NULL;
    }

    if ( ! bit_is_set(pool->flags, AP_NET_POOL_FLAGS_TCP) ) /* do manual bind() for UDP */
    {
        memset(&conn->local, 0, sizeof(conn->local));
        conn->local.af = conn->remote.af;

        if ( -1 == bind(conn->fd, (struct sockaddr *)&conn->local, sizeof(conn->local)))
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "bind()");
            ap_net_connection_unlock(conn);

            return NULL;
        }
    }

    if ( 0 != connect(conn->fd, (struct sockaddr *)&conn->remote, sizeof(conn->remote)) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "connect()");

        goto lblerror;
    }

    slen = sizeof(conn->local);

    if ( 0 != getsockname(conn->fd, (struct sockaddr *)&conn->local, &slen)) /* getting our side addr and port */
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "getsockname()");

        goto lblerror;
    }

    fcntl(conn->fd, F_SETFL, fcntl(conn->fd, F_GETFL) | O_NONBLOCK);

    conn->state = AP_NET_ST_CONNECTED;

    if ( conn->parent->poller != NULL && ! ap_net_conn_pool_poller_add_conn(conn->parent, conn->idx) )
        goto lblerror;

    if( ! bit_is_set(conn->flags, AP_NET_CONN_FLAGS_UDP_IN) )
    {
        if ( pool->callback_func != NULL )
            pool->callback_func(conn, AP_NET_SIGNAL_CONN_CONNECTED);

        if (ap_log_debug_level)
            ap_log_debug_log("* Outbound connection #%d initiated\n", conn->idx);
    }

    ap_net_connection_unlock(conn);

    return conn;

lblerror:
    close(conn->fd);
    conn->fd = -1;
    ap_net_connection_unlock(conn);
    bit_clear(conn->state, AP_NET_ST_CONNECTED);
    return NULL;
}

/*************************************************************/
static int sanity_check(struct ap_net_conn_pool_t *pool, int port, int expire_in_ms, unsigned flags)
{
    struct ap_net_connection_t *conn;
    int conn_idx;


    ap_error_clear();

    if(expire_in_ms < 0)
        return -1;

    if (pool->used_slots == pool->max_connections)
    {
        pool->stat.queue_full_count++;

        ap_error_set(_func_name, AP_ERRNO_CONNLIST_FULL);

        return -1;
    }

    for ( conn_idx = 0; conn_idx < pool->max_connections; ++conn_idx ) /* finding free slot */
        if ( ! (pool->conns[conn_idx].state & AP_NET_ST_CONNECTED) )
            break;

    if ( conn_idx == pool->max_connections )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_CONNLIST_FULL, "Internal structure error. Free slots counter != actual free slots");
        return -1;
    }

    conn = &pool->conns[conn_idx];

    ap_net_conn_pool_connection_pre_connect(pool, conn_idx, flags);

    if ( ! ap_net_connection_lock(conn) )
        return -1;

    if ( expire_in_ms )
        ap_utils_timespec_set(&conn->expire, AP_UTILS_TIME_SET_FROM_NOW, expire_in_ms);
    else
        ap_utils_timespec_clear(&conn->expire);

    return conn_idx;
}

/* ********************************************************************** */
/** \brief Creates new outgoing connection in pool, using string representation of address, autodetecting it's type if necessary
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param flags unsigned - bit set of AP_NET_CONN_FLAGS_*
 * \param address_str char* string with remote text or IP/IP6 address
 * \param af int - Address family (AF_INET or AF_INET6)
 * \param port - port to connect
 * \param expire_in_ms - expiration time in milliseconds. 0 if persistent
 * \return NULL on error. connection pointer if OK.
 *
 * if af is AF_INET6 the string should be numeric IPv6 address or textual name conversion should be in IPv6.
 * Same applies for AF_INET and IPv4
 * Autodetection is performed if it's have other value
 * Note: the pool's AF type is not relevant here as we doing outgoing connection
 */
struct ap_net_connection_t *ap_net_conn_pool_connect_straddr(struct ap_net_conn_pool_t *pool, unsigned flags, const char *address_str, int af, int port, int expire_in_ms)
{
    struct ap_net_connection_t *conn;
    int conn_idx;


    conn_idx = sanity_check(pool, port, expire_in_ms, flags);

    if (conn_idx == -1)
        return NULL;

    conn = &pool->conns[conn_idx];

    if ( af == AF_INET6 )
    {
        if( -1 == inet_pton(AF_INET6, address_str, &conn->remote.addr6.sin6_addr) )
        {
            ap_error_set_custom(_func_name, "bad IPv6: %s", address_str);
            ap_net_connection_unlock(conn);

            return NULL;
        }

        conn->remote.addr6.sin6_family = AF_INET6;
        conn->remote.addr6.sin6_port = htons(port);
    }

    else if ( af == AF_INET )
    {
        if ( ! inet_aton(address_str, &conn->remote.addr4.sin_addr) )
        {
             ap_error_set_custom(_func_name, "bad IP: %s", address_str);
             ap_net_connection_unlock(conn);
             return NULL;
        }

        conn->remote.addr4.sin_family = AF_INET;
        conn->remote.addr4.sin_port = htons(port);
    }

    else /* trying to detect */
    {
        if( -1 != inet_pton(AF_INET6, address_str, &conn->remote.addr6.sin6_addr) )
        {
            conn->remote.addr6.sin6_family = AF_INET6;
            conn->remote.addr6.sin6_port = htons(port);
        }

        else if( -1 != inet_pton(AF_INET, address_str, &conn->remote.addr4.sin_addr) )
        {
            conn->remote.addr4.sin_family = AF_INET;
            conn->remote.addr4.sin_port = htons(port);
        }

        else
        {
            ap_error_set_custom(_func_name, "can't autodetect address type: %s", address_str);
            ap_net_connection_unlock(conn);

            return NULL;
        }
    }

    return ap_net_conn_pool_do_connect(pool, conn_idx);
}

/* ********************************************************************** */
/** \brief Creates new outgoing connection in pool, using numeric IPv4 address
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param flags unsigned - bit set of AP_NET_CONN_FLAGS_*
 * \param address4 in_addr_t - remote numeric IP address. see inet_aton() man
 * \param port - port to connect
 * \param expire_in_ms - expiration time in milliseconds. 0 if persistent
 * \return NULL on error. connection pointer if OK.
 *
 * Address and port are automatically converted to network byte order by hton*()
 * Note: the pool's AF type is not relevant here as we doing outgoing connection
 */
struct ap_net_connection_t *ap_net_conn_pool_connect_ip4(struct ap_net_conn_pool_t *pool, unsigned flags, in_addr_t address4, int port, int expire_in_ms)
{
    struct ap_net_connection_t *conn;
    int conn_idx;


    conn_idx = sanity_check(pool, port, expire_in_ms, flags);

    if (conn_idx == -1)
        return NULL;

    conn = &pool->conns[conn_idx];

    conn->remote.addr4.sin_family = AF_INET;
    conn->remote.addr4.sin_port = htons(port);
    conn->remote.addr4.sin_addr.s_addr = htonl(address4);

    return ap_net_conn_pool_do_connect(pool, conn_idx);
}

/* ********************************************************************** */
/** \brief Creates new outgoing connection in pool, using numeric IPv6 address
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param flags unsigned - bit set of AP_NET_CONN_FLAGS_*
 * \param address6 struct in6_addr * - remote numeric IPv6 address
 * \param port - port to connect
 * \param expire_in_ms - expiration time in milliseconds. 0 if persistent
 * \return NULL on error. connection pointer if OK.
 *
 * Port is automatically converted to network byte order by hton*()
 * Note: the pool's AF type is not relevant here as we doing outgoing connection
 */
struct ap_net_connection_t *ap_net_conn_pool_connect_ip6(struct ap_net_conn_pool_t *pool, unsigned flags, struct in6_addr *address6, int port, int expire_in_ms)
{
    struct ap_net_connection_t *conn;
    int conn_idx;


    conn_idx = sanity_check(pool, port, expire_in_ms, flags);

    if (conn_idx == -1)
        return NULL;

    conn = &pool->conns[conn_idx];

    /*TODO: STUB. should check if it is right method: */
    conn->remote.addr6.sin6_family = AF_INET6;
    conn->remote.addr6.sin6_port = htons(port);
    memcpy(&conn->remote.addr6.sin6_addr, address6, sizeof(struct in6_addr));

    return ap_net_conn_pool_do_connect(pool, conn_idx);
}
