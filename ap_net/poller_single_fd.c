/** \file ap_net/poller_single_fd.c
 * \brief Part of AP's toolkit. Networking module, Poller: Easy event poll procedures for single socket descriptor
 */
#define AP_NET_CONN_POOL

#include "ap_net.h"
#include "../ap_error/ap_error.h"
#include <unistd.h>

static const char *_func_name = "ap_net_poller_single_fd()";

/* ********************************************************************** */
/** \brief Do poll task for given socket descriptor. It's a simpler interface to epoll()
 *
 * \param fd int - open socket descriptor
 * \return int -1 if general error, 0 if no events, otherwise state bits AP_NET_POLLER_ST_*
 *
 * Doing the round-robin polling on registered socket descriptors.
 * So each call to this function returning status bit field for the next descriptor in list.
 * bits are:
 * AP_NET_POLLER_ST_ERROR - Error detected
 * AP_NET_POLLER_ST_IN - Data available from peer
 * AP_NET_POLLER_ST_OUT - Socket ready to send data
 */
int ap_net_poller_single_fd(int fd)
{
    int poll_status;
    int epoll_fd;
    struct epoll_event ev;


    epoll_fd = epoll_create(1);

    if (epoll_fd == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return -1;
    }

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "epoll_ctl() add listen_socket_fd");
        close(epoll_fd);

        return -1;
    }

    poll_status = epoll_wait(epoll_fd, &ev, 1, 0);

    if (poll_status == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return -1;
    }

    close(epoll_fd);

    poll_status = 0;

    if ( ev.events & EPOLLIN ) /* incoming data available */
        poll_status |= AP_NET_POLLER_ST_IN;

    if ( ev.events & EPOLLOUT ) /* can send */
        poll_status |= AP_NET_POLLER_ST_OUT;

    if ( ev.events & (EPOLLERR | EPOLLHUP ) ) /* connection's ERROR */
        poll_status |= AP_NET_POLLER_ST_ERROR;

    return poll_status;
}

