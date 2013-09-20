#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_poll()";

/* ********************************************************************** */
/** \brief do poll task, adding, removing connection(s) and/or notifying user callback of data available
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int
 *
 */
int ap_net_conn_pool_poll(struct ap_net_conn_pool_t *pool)
{
    int events_count;
    int event_idx;
    struct epoll_event ev;
    struct ap_net_poll_t *poller;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    poller = pool->poller;

    events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);

    if (events_count == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return 0;
    }

    for ( event_idx = 0; event_idx < events_count; ++event_idx)
    {
        if ( poller->events[event_idx].data.fd == poller->listen_socket_fd) /*  listener socket? */
        {
            conn = ap_net_conn_pool_accept_connection(pool);

            if (conn == NULL)
                return 0;

            if ( ! ap_net_conn_pool_poller_add_conn(pool, conn->idx) )
                return 0;
        }
        else /*  ordinary connection */
        {
            conn = ap_net_conn_pool_get_conn_by_fd(pool, poller->events[event_idx].data.fd);

            if ( conn == NULL ) /* this should be the point of starting the fearful debugging */
            {
                ev.events = EPOLLIN;
                ev.data.fd = poller->events[event_idx].data.fd;

                if ( -1 == epoll_ctl(poller->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev) )
                {
                    ap_error_set(_func_name, AP_ERRNO_SYSTEM);
                    return 0;
                }

                continue;
            }

            if ( poller->events[event_idx].events & (EPOLLERR | EPOLLHUP ) ) /* connection's ERROR? */
            {
                conn->state |= AP_NET_ST_ERROR;

                ap_net_conn_pool_close_connection(pool, conn->idx, NULL);

                continue;
            }

            if ( poller->events[event_idx].events & EPOLLIN ) /*  data available for reading */
            {
                if ( 0 < ap_net_conn_pool_recv(pool, conn->idx) )
                     if ( pool->data_func != NULL )
                          pool->data_func(conn);
            }
        }
    } /*  for (n = 0; n < nfds; ++n) */

    return 1;
}

