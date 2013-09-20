#define AP_NET_POLLER

#include "../ap_net.h"
#include "../../ap_error/ap_error.h"
#include <unistd.h>

static const char *_func_name = "ap_net_poller_create()";

/* ********************************************************************** */
/*
 * listen_socket_fd can be -1 for client side
 * parent can be NULL for stand-alone poller structure creation
 */
struct ap_net_poll_t *ap_net_poller_create(int listen_socket_fd, int max_connections)
{
    struct ap_net_poll_t *poller;
    struct epoll_event ev;
    int epoll_fd;


    /* maybe we should switch to something like 'ivykis' - library for asynchronous I/O readiness notification */

    ap_error_clear();

    if (listen_socket_fd >= 0)
        ++max_connections;

    epoll_fd = epoll_create(max_connections);

    if (epoll_fd == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return NULL;
    }

    poller = malloc(sizeof(struct ap_net_poll_t));

    if ( poller == NULL )
    {
        close(epoll_fd);
        return NULL;
    }

    poller->events = malloc(max_connections * sizeof(struct epoll_event));

    if ( poller->events == NULL )
    {
        close(epoll_fd);
        free(poller);
        return NULL;
    }

    poller->max_events = max_connections;
    poller->epoll_fd = epoll_fd;
    poller->listen_socket_fd = listen_socket_fd;
    poller->last_event_index = -1;
    poller->events_count = 0;

    if ( listen_socket_fd >= 0 )
    {
        ev.events = EPOLLIN;
        ev.data.fd = listen_socket_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &ev) == -1)
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "epoll_ctl() add listen_socket_fd");
            close(epoll_fd);
            free(poller->events);
            free(poller);

            return NULL;
        }
    }

    return poller;
}
