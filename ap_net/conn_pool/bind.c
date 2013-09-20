#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"
#include <unistd.h>

static const char *_func_name = "ap_net_conn_pool_bind()";

/* ********************************************************************** */
int ap_net_conn_pool_bind(struct ap_net_conn_pool_t *pool, char *address, int port, int max_tries, int retry_sleep)
{
    ap_error_clear();

    if ( address != NULL ) /* pool->listen_addr should be already filled with appropriate data */
    {
        memset (&pool->listen_addr, 0, sizeof (pool->listen_addr));
        pool->listen_addr.sin_family = AF_INET;
        pool->listen_addr.sin_port = port;

        if ( ! inet_aton(address, &pool->listen_addr.sin_addr) )
        {
            ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "bad IP: %s", address);
            return -1;
        }
    }

    pool->listen_sock = socket( AF_INET, pool->type == AP_NET_CTYPE_TCP ? SOCK_STREAM : SOCK_DGRAM, 0 );

    if ( -1 == pool->listen_sock )
    {
        ap_error_set(_func_name, AP_ERRNO_SYSTEM);
        return -1;
    }

    for(;;)
    {
        if ( -1 != bind( pool->listen_sock, (struct sockaddr *)&pool->listen_addr, sizeof(pool->listen_addr) ) )
            break; /*  bind OK */

        if ( --max_tries == 0 )
        {
            ap_error_set(_func_name, AP_ERRNO_SYSTEM);
            close(pool->listen_sock);
            pool->listen_sock = -1;
            return -1;
        }

        ap_log_do_syslog(LOG_ERR, "%s: %m: retries left: %d, sleeping for %d sec(s)", _func_name, max_tries, retry_sleep);

        sleep(retry_sleep);
    }

    return pool->listen_sock;
}

