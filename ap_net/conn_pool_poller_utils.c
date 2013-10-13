#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

/* ********************************************************************** */
/** \brief Adds new socket handle to the pool's poller list
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int - Connection index
 * \return int - True on success, False on error
 *
 * This should be safe to call from outside of library, but it's really internal thing
 * The new poller is created if non-existent
 */
int ap_net_conn_pool_poller_add_conn(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct epoll_event ev;


    ap_error_clear();

    if ( pool->poller == NULL )
    	return ap_net_conn_pool_poller_create(pool);

    ev.events = EPOLLIN;
    ev.data.fd = pool->conns[conn_idx].fd;

    if (epoll_ctl(pool->poller->epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
    {
        ap_error_set("ap_net_conn_pool_poller_add_conn()", AP_ERRNO_SYSTEM);
        return 0;
    }

    return 1;
}

/* ********************************************************************** */
/** \brief Removes socket handle from the pool's poller list
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int - Connection index
 * \return int - True on success, False on error
 *
 * This should be safe to call from outside of library, but it's really internal thing
 */
int ap_net_conn_pool_poller_remove_conn(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct epoll_event ev;


    ap_error_clear();

    if ( pool->poller == NULL )
    	return 0;

    ev.events = EPOLLIN;
    ev.data.fd = pool->conns[conn_idx].fd;

    /* ENOENT = 'No such file or directory'. Means that our fd is maybe from already closed conn or removed lately, so we can ignore error */
    if (epoll_ctl(pool->poller->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1 && errno != ENOENT )
    {
        ap_error_set("ap_net_conn_pool_poller_remove_conn()", AP_ERRNO_SYSTEM);
        return 0;
    }

    return 1;
}

/* ********************************************************************** */
/** \brief Creates poller for the given pool
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int - True on success, False on error
 *
 * Existing active connections and listener socket handles are added to the new poller automatically
 */
int ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool)
{
    int conn_idx;


    ap_error_clear();

    pool->poller = ap_net_poller_create(pool->listener.sock, pool->max_connections);

    if ( pool->poller == NULL )
      return 0;

    for ( conn_idx = 0; conn_idx < pool->max_connections; ++conn_idx )
    {
        if ( ! (pool->conns[conn_idx].state & AP_NET_ST_CONNECTED) || (pool->conns[conn_idx].state & (AP_NET_ST_ERROR | AP_NET_ST_DISCONNECTION)) )
            continue;

        if ( ! ap_net_conn_pool_poller_add_conn(pool, conn_idx))
            return 0;
    }

    return 1;
}

