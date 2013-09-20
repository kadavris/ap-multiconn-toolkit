#define AP_NET_CONN_POOL

#include "../ap_net.h"
#include "../../ap_error/ap_error.h"

static const char *_func_name = "ap_net_poller_poll()";

/* ********************************************************************** */
/** \brief do very simple, cyclic poll task
 *
 * \param poller struct ap_net_poller_t *
 * \return int -1 if general error, 0 if no events or state bits
 * bit 0 if listener socket event
 * bit 1 if some data arrived
 * bit 2 if socket error
 *
 */
int ap_net_poller_poll(struct ap_net_poll_t *poller)
{
    int state;


    ap_error_clear();

    if ( poller->events_count <= 0 || poller->last_event_index < 0 || poller->last_event_index >= poller->events_count )
    {
        poller->last_event_index = -1;
        poller->events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);
    }

    if (poller->events_count == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return -1;
    }

    if (poller->events_count == 0)
        return 0;

    poller->last_event_index++;
    state = 0;

    if ( poller->events[poller->last_event_index].data.fd == poller->listen_socket_fd ) /*  listener socket */
        state = 1;

    if ( poller->events[poller->last_event_index].events & EPOLLIN ) /* data available */
        state |= 2;

    if ( poller->events[poller->last_event_index].events & (EPOLLERR | EPOLLHUP ) ) /* connection's ERROR */
        state |= 4;

    return state;
}

