/* Part of AP's Toolkit
 * Networking module
 * ap_net/conn_pool_listener_create.c
 */

#include "conn_pool_internals.h"
#include <fcntl.h>
#include <unistd.h>

static const char *_func_name = "ap_net_conn_pool_listener_create()";

/* ********************************************************************** */
/** \brief Creates and starts listener socket for incoming connections
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param max_tries int - Re-try to bind() to a local address how many times
 * \param retry_sleep int - sleep time in second between re-trys
 * \return int - listener socket handle on OK or -1 on error
 *
 * max_tries and retry_sleep used to cyclically attempt to bind() to registered address.
 * This is usable if software is run on early system startup when interfaces can be uninitialized for a while.
 * For the TCP pool it also issues listen() system call.
 * Socket is set to non-blocking mode after bind() so listen() returns immediately.
 * Also poller is automatically created if not exists and re-created if found. Socket handle is registered in there automaticaly.
 */
int ap_net_conn_pool_listener_create(struct ap_net_conn_pool_t *pool, int max_tries, int retry_sleep)
{
	size_t addr_len;
	struct sockaddr *addr;


    ap_error_clear();

    if ( -1 != pool->listener.sock )
    {
        ap_error_set_custom(_func_name, "listener is already active");
        return -1;
    }

    pool->listener.sock = socket( pool->flags & AP_NET_POOL_FLAGS_IPV6 ? AF_INET6 : AF_INET, pool->flags & AP_NET_POOL_FLAGS_TCP ? SOCK_STREAM : SOCK_DGRAM, 0 );

    if ( -1 == pool->listener.sock )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "socket()");
        return -1;
    }

    if (pool->flags & AP_NET_POOL_FLAGS_IPV6)
    {
    	addr = (struct sockaddr *)(&pool->listener.addr6);
    	addr_len = sizeof(struct sockaddr_in6);
    }
    else
    {
    	addr = (struct sockaddr *)(&pool->listener.addr4);
    	addr_len = sizeof(struct sockaddr_in);
    }

    for(;;)
    {
        if ( -1 != bind( pool->listener.sock, addr, addr_len ) )
            break; /*  bind OK */

        if ( --max_tries == 0 )
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "bind()");
            close(pool->listener.sock);
            pool->listener.sock = -1;
            return -1;
        }

        ap_log_do_syslog(LOG_ERR, "%s: %m: retries left: %d, sleeping for %d sec(s)", _func_name, max_tries, retry_sleep);

        sleep(retry_sleep);
    }

   	fcntl(pool->listener.sock, F_SETFL, fcntl(pool->listener.sock, F_GETFL) | O_NONBLOCK);

   	if ( pool->poller != NULL ) /* recreating. ugly, but fine for now */
   	{
   		ap_net_poller_destroy(pool->poller);
   		pool->poller = NULL;
   	}

	if ( ! ap_net_conn_pool_poller_create(pool))
	{
        close(pool->listener.sock);
        pool->listener.sock = -1;
        return -1;
	}

    if ( (pool->flags & AP_NET_POOL_FLAGS_TCP)
    		&& 0 != listen(pool->listener.sock, pool->max_connections) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "listen()");
        close(pool->listener.sock);
        pool->listener.sock = -1;
        return -1;
    }

    return pool->listener.sock;
}

