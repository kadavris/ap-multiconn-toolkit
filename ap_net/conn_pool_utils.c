/** \file ap_net/conn_pool_utils.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Miscellaneous procedures
 */
#include <fcntl.h>
#include <unistd.h>
#include "conn_pool_internals.h"

/* **********************************************************************
 * internal common locking procedure for pools and connections
 */
static int lock(unsigned int *state)
{
    int lock_try;
#define WAIT_MSEC 50

    for ( lock_try = 1; *state & AP_NET_ST_BUSY; ++lock_try )
    {
        if ( lock_try == 10000 / WAIT_MSEC ) /* 10 sec to wait. pretty enough for very busy system with heavy load */
            return 0;

        usleep(WAIT_MSEC * 1000); /* to usec */
    }

    *state |= AP_NET_ST_BUSY;

    return 1;
}

/* ********************************************************************** */
/** \brief Locks connection against I/O operations
 *
 * \param conn struct ap_net_connection_t *
 * \return int - true - successful lock, false - timeout. probably stale lock
 */
int ap_net_connection_lock(struct ap_net_connection_t *conn)
{
	return lock(&conn->state);
}

/* ********************************************************************** */
/** \brief Unlocks connection for I/O operations
 *
 * \param conn struct ap_net_connection_t *
 * \return void
 */
void ap_net_connection_unlock(struct ap_net_connection_t *conn)
{
    bit_clear(conn->state, AP_NET_ST_BUSY);
}

/* ********************************************************************** */
/** \brief Locks pool against I/O operations
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int - true - successful lock, false - timeout. probably stale lock
 */
int ap_net_conn_pool_lock(struct ap_net_conn_pool_t *pool)
{
	return lock(&pool->state);
}

/* ********************************************************************** */
/** \brief Unlocks pool for I/O operations
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return void
 */
void ap_net_conn_pool_unlock(struct ap_net_conn_pool_t *pool)
{
    bit_clear(pool->state, AP_NET_ST_BUSY);
}

/* ********************************************************************** */
/** \brief Finds first connection in pool's list using the specified socket descriptor
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param fd int - socket handle value to find.
 * \return struct ap_net_connection_t * - connection pointer if found, NULL if not
 *
 */
struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_fd(struct ap_net_conn_pool_t *pool, int fd)
{
    int i;


    for ( i = 0; i < pool->max_connections; ++i)
        if ( pool->conns[i].fd == fd )
            return &pool->conns[i];

    return NULL;
}

/* ********************************************************************** */
/** \brief Finds first connection in pool's list using the specified local or remote port
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param port int - port value to find. should be in network order
 * \param is_local - if true then search in local address records. In remote records otherwise
 * \return struct ap_net_connection_t * - connection pointer if found, NULL if not
 */
struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_port(struct ap_net_conn_pool_t *pool, in_port_t port, int is_local)
{
    int i;
    int ip6;


    for ( i = 0; i < pool->max_connections; ++i)
    {
        ip6 = (is_local ? pool->conns[i].local.af : pool->conns[i].remote.af) == AF_INET6;

        if ( port == (is_local ? (ip6 ? pool->conns[i].local.addr6.sin6_port : pool->conns[i].local.addr4.sin_port)
        					   : (ip6 ? pool->conns[i].remote.addr6.sin6_port : pool->conns[i].remote.addr4.sin_port)) )
            return &pool->conns[i];
    }

    return NULL;
}

/* ********************************************************************** */
/** \brief Finds first connection in pool's list using the specified local or remote address and port
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param ss struct sockaddr_storage *ss - pointer to the address structure to search for
 * \param is_local - if true then search in local address records. In remote records otherwise
 * \return struct ap_net_connection_t * - connection pointer if found, NULL if not
 */
struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_address(struct ap_net_conn_pool_t *pool, struct sockaddr_storage *ss, int is_local)
{
	struct ap_net_connection_t *conn;
	void *address;
	int port, ip6;
	int i;


    ip6 = (ss->ss_family == AF_INET6);
	address = (ip6 ? (void*)&((struct sockaddr_in6 *)ss)->sin6_addr : (void*)&((struct sockaddr_in *)ss)->sin_addr);
	port = (ip6 ? ((struct sockaddr_in6 *)ss)->sin6_port : ((struct sockaddr_in *)ss)->sin_port);

    for ( i = 0; i < pool->max_connections; ++i)
    {
    	conn = &pool->conns[i];

        if ( port == (is_local ? (ip6 ? conn->local.addr6.sin6_port : conn->local.addr4.sin_port)
        			           : (ip6 ? conn->remote.addr6.sin6_port : conn->remote.addr4.sin_port)
        			 )
             && 0 == memcmp(address,
            		 	    	(is_local ? (ip6 ? (void*)&conn->local.addr6.sin6_addr : (void*)&conn->local.addr4.sin_addr)
            		 	    			  : (ip6 ? (void*)&conn->remote.addr6.sin6_addr : (void*)&conn->remote.addr4.sin_addr)
            		 	    	),
            		 	    	ip6 ? sizeof(struct in6_addr) : sizeof(struct in_addr)
            			   )
        	)
            return conn;
    }

    return NULL;
}

/* ********************************************************************** */
/** \brief Destroys connection.
 *
 * \param conn struct ap_net_connection_t *
 * \param free_this int - if true then connection's structure memory will be freed too. False for only internal parts of structure freeing
 * \return void
 */
void ap_net_connection_destroy(struct ap_net_connection_t *conn, int free_this)
{
	if (conn->parent != NULL && conn->parent->callback_func != NULL )
		conn->parent->callback_func(conn, AP_NET_SIGNAL_CONN_DESTROYING );

	free(conn->buf);

	if ( free_this )
    	free(conn);
}

/* ********************************************************************** */
/** \brief Clears connection's receiving buffer and it's supporting variables.
 *
 * \param conn struct ap_net_connection_t *
 * \param fill_char int - if in range of 0 <= fill_char <= 255 then buffer is filled with this byte
 * \return void
 */
void ap_net_connection_buf_clear(struct ap_net_connection_t *conn, int fill_char)
{
	conn->buffill = conn->bufpos = 0;

	if ( fill_char >= 0 && fill_char < 256 )
		memset(conn->buf, fill_char, conn->bufsize);
}

/* ********************************************************************** */
/** \brief Destroys pool. closes listener socket and connections and freeing allocated memory
 *
 * \param pool struct ap_net_conn_pool_t *
 * \param free_this int - if true then pool's structure memory will be freed too. False for only internal parts of structure freeing
 * \return void
 */
void ap_net_conn_pool_destroy(struct ap_net_conn_pool_t *pool, int free_this)
{
    int i;


    if ( pool->listener.sock != -1 )
    	close(pool->listener.sock);

    if ( pool->poller != NULL )
    	ap_net_poller_destroy(pool->poller);

    for ( i = 0; i < pool->max_connections; ++i)
    {
        if ( pool->conns[i].state & AP_NET_ST_CONNECTED )
             ap_net_conn_pool_close_connection(pool, i);

        ap_net_connection_destroy(&pool->conns[i], 0);
    }

    free(pool->conns);

    if ( free_this )
    	free(pool);
}

/* ********************************************************************** */
/** \brief Finds first available non-connected slot in pool's list
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return struct ap_net_connection_t * - NULL on error. connection pointer if OK
 */
struct ap_net_connection_t *ap_net_conn_pool_find_free_slot(struct ap_net_conn_pool_t *pool)
{
    struct ap_net_connection_t *conn;
    int conn_idx;


    if (pool->used_slots == pool->max_connections)
    {
        pool->stat.queue_full_count++;

        ap_error_set("ap_net_conn_pool_find_free_slot()", AP_ERRNO_CONNLIST_FULL);

        return NULL;
    }

    conn = NULL;

    for ( conn_idx = 0; conn_idx < pool->max_connections; ++conn_idx ) /*  finding free slot */
        if ( ! (pool->conns[conn_idx].state & AP_NET_ST_CONNECTED) )
        {
            conn = &pool->conns[conn_idx];
            break;
        }

    return conn;
}

/* ********************************************************************** */
/*  internal semi-failsafe recv() */
int ap_net_recv(int sh, void *buf, int size, int non_blocking)
{
    int retval;
    int flags;


    ap_error_clear();

    retval = fcntl(sh, F_GETFL);

    if ( retval != -1 )
    {
    	flags = (non_blocking ? MSG_DONTWAIT : 0) | MSG_NOSIGNAL;
        retval = recv(sh, buf, size, flags);
    }

    if ( retval < 0 )
        ap_error_set_detailed("ap_net_recv", AP_ERRNO_SYSTEM, "sock %d", sh);

    return retval;
}

/* ********************************************************************** */
/*  internal semi-failsafe send() */
int ap_net_send(int sh, void *buf, int size, int non_blocking)
{
    int retval;
    int flags;


    ap_error_clear();

    retval = fcntl(sh, F_GETFL);

    if ( retval != -1 )
    {
    	flags = (non_blocking ? MSG_DONTWAIT : 0) | MSG_NOSIGNAL;
        retval = send(sh, buf, size, flags);
    }

    if ( retval <= 0 )
        ap_error_set_detailed("ap_net_send", AP_ERRNO_SYSTEM, "sock %d", sh);

    return retval;
}

