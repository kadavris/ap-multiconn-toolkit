#define AP_NET_CONN_POOL

#include <fcntl.h>
#include <unistd.h>
#include "conn_pool_internals.h"
#include "../../ap_error/ap_error.h"

/* ********************************************************************** */
int ap_net_conn_pool_lock(struct ap_net_conn_pool_t *pool)
{
    int lock_try;


    for ( lock_try = 1; pool->status & AP_NET_ST_BUSY; ++lock_try )
    {
        if ( lock_try == 10 )
            return 0;

        usleep(10000);
    }

    pool->status |= AP_NET_ST_BUSY;

    return 1;
}

/* ********************************************************************** */
void ap_net_conn_pool_unlock(struct ap_net_conn_pool_t *pool)
{
    pool->status &= AP_NET_ST_BUSY ^ 0xff;
}

/* ********************************************************************** */
struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_fd(struct ap_net_conn_pool_t *pool, int fd)
{
    int i;


    for ( i = 0; i < pool->max_connections; ++i)
        if ( pool->conns[i].fd == fd )
            return &pool->conns[i];

    return NULL;
}

/* ********************************************************************** */
/*  internal semi-failsafe recv() */
int ap_net_tcprecv(int sh, void *buf, int size)
{
	int retval;


    ap_error_clear();

    retval = fcntl(sh, F_GETFL);

    if ( retval != -1 )
      retval = recv(sh, buf, size, MSG_DONTWAIT);

    if ( retval <= 0 )
    	ap_error_set_detailed("ap_net_tcprecv", AP_ERRNO_SYSTEM, "sock %d", sh);

    return retval;
}

/* ********************************************************************** */
int ap_net_tcpsend(int sh, void *buf, int size)
/*  internal semi-failsafe send() */
{
	int retval;


	ap_error_clear();

    retval = fcntl(sh, F_GETFL);

    if ( retval != -1 )
    	retval = send(sh, buf, size, MSG_DONTWAIT);

    if ( retval <= 0 )
    	ap_error_set_detailed("ap_net_tcpsend", AP_ERRNO_SYSTEM, "sock %d", sh);

    return retval;
}

