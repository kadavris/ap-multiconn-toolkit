/** \file ap_net/poller_create.c
 * \brief Part of AP's toolkit. Networking module, Poller: creation and initial setup procedures
 */
#define AP_NET_POLLER

#include "ap_net.h"
#include "../ap_error/ap_error.h"
#include <unistd.h>

static const char *_func_name = "ap_net_poller_create()";

/* ********************************************************************** */
/** \brief Creates poller structure, adding the listener socket if set
 *
 * \param listen_socket_fd int - listener socket, -1 if not used
 * \param max_connections int - maximum connections/events count. > 0
 * \return struct ap_net_poll_t * - pointer to the newly created structure or NULL on error
 *
 * This is a helper function for ap_net_conn_pool_poller_create(), but can be used to create stand-alone poller
 */
struct ap_net_poll_t *ap_net_poller_create(int listen_socket_fd, int max_connections)
{
    struct ap_net_poll_t *poller;
    struct epoll_event ev;
    int epoll_fd;


    /* maybe we should switch to something like 'ivykis' - library for asynchronous I/O readiness notification */

    ap_error_clear();

    if (listen_socket_fd > 0)
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
    poller->debug = 0;
    poller->emit_old_data_signal = 1;

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

/* ********************************************************************** */
void ap_net_poller_destroy(struct ap_net_poll_t *poller)
{
    close(poller->epoll_fd);
    free(poller->events);
    free(poller);
}
