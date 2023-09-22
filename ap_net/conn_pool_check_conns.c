/** \file ap_net/conn_pool_check_conns.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: connections state checking procedures
 */
#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_check_conns()";

/* ********************************************************************** */
/** \brief Polls connections and closes all with errors
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int 1 - all OK (even if some connections was removed), 0 - some general error occured. -1 - listener socket error
 *
 * Can be used in sigaction() EPIPE handler to prevent dumping when connection dropped unexpectedly.
 * Recommended to use ap_net_conn_pool_poll() to do full scale processing with blackjack and hookers
 */
int ap_net_conn_pool_check_conns(struct ap_net_conn_pool_t *pool)
{
    int events_count;
    int event_idx;
    struct ap_net_poll_t *poller;
    struct ap_net_connection_t *conn;
    struct epoll_event ev;


    ap_error_clear();

    if ( pool->poller == NULL && ! ap_net_conn_pool_poller_create(pool) )
        return 0;

    poller = pool->poller;

    events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);

    if (events_count == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return 0;
    }

    for ( event_idx = 0; event_idx < events_count; ++event_idx)
    {
        if ( 0 == (poller->events[event_idx].events & (EPOLLERR | EPOLLHUP )) ) /* no errors */
            continue;

        if ( poller->events[event_idx].data.fd == poller->listen_socket_fd )
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "listener died");
            return -1;
        }

        conn = ap_net_conn_pool_get_conn_by_fd(pool, poller->events[event_idx].data.fd);

        if ( conn == NULL ) /* this should be the starting point of the fearful debugging */
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

        conn->state |= AP_NET_ST_ERROR;

        ap_net_conn_pool_close_connection(pool, conn->idx);
    } /* for ( event_idx = 0; event_idx < events_count; ++event_idx) */

    return 1;
}

