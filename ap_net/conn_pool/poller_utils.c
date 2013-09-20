#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

/* ********************************************************************** */
int ap_net_conn_pool_poller_add_conn(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct epoll_event ev;


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
int ap_net_conn_pool_poller_remove_conn(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct epoll_event ev;


    ev.events = EPOLLIN;
    ev.data.fd = pool->conns[conn_idx].fd;

    if (epoll_ctl(pool->poller->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1)
    {
        ap_error_set("ap_net_conn_pool_poller_remove_conn()", AP_ERRNO_SYSTEM);
        return 0;
    }

    return 1;
}

/* ********************************************************************** */
int ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool)
{
    int conn_idx;


    pool->poller = ap_net_poller_create(pool->listen_sock, pool->max_connections);

    if ( pool->poller == NULL )
      return 0;

    for ( conn_idx = 0; conn_idx < pool->max_connections; ++conn_idx )
    {
        if ( ! (pool->conns[conn_idx].state & AP_NET_ST_CONNECTED) )
            continue;

        if ( ! ap_net_conn_pool_poller_add_conn(pool, conn_idx))
            return 0;
    }

    return 1;
}

