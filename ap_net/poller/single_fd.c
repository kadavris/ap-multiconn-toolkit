#define AP_NET_CONN_POOL

#include "../ap_net.h"
#include "../../ap_error/ap_error.h"
#include <unistd.h>

static const char *_func_name = "ap_net_poller_single_fd()";

/* ********************************************************************** */
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

    poll_status = epoll_wait(epoll_fd, &ev, 1, 0);

    if (poll_status == -1)
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return -1;
    }

    close(epoll_fd);

    return ev.events;
}

