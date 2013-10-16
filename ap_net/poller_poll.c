/** \file ap_net/poller_poll.c
 * \brief Part of AP's toolkit. Networking module, Poller: standalone poller events processing procedures
 */
#define AP_NET_CONN_POOL

#include "ap_net.h"
#include "../ap_error/ap_error.h"

static const char *_func_name = "ap_net_poller_poll()";

/* ********************************************************************** */
/** \brief Do very simple, cyclic poll task, returning status bit field for one connection at a time
 *
 * \param poller struct ap_net_poller_t *
 * \return int -1 if general error, 0 if no events, otherwise state bits AP_NET_POLLER_ST_*
 *
 * Doing the round-robin polling on registered socket descriptors.
 * So each call to this function returning status bit field for the next descriptor in list.
 * bits are:
 * AP_NET_POLLER_ST_LISTENER - Status returned is for listener socket
 * AP_NET_POLLER_ST_ERROR - Error detected
 * AP_NET_POLLER_ST_IN - Data available from peer
 * AP_NET_POLLER_ST_OUT - Socket ready to send data
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
        state = AP_NET_POLLER_ST_LISTENER;

    if ( poller->events[poller->last_event_index].events & EPOLLIN ) /* incoming data available */
        state |= AP_NET_POLLER_ST_IN;

    if ( poller->events[poller->last_event_index].events & EPOLLOUT ) /* can send */
        state |= AP_NET_POLLER_ST_OUT;

    if ( poller->events[poller->last_event_index].events & (EPOLLERR | EPOLLHUP ) ) /* connection's ERROR */
        state |= AP_NET_POLLER_ST_ERROR;

    return state;
}

