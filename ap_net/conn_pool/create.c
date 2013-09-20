#define AP_NET_CONN_POOL

#include "../ap_net.h"
#include "conn_pool_internals.h"
#include "../../ap_error/ap_error.h"

static const char *_func_name = "ap_net_conn_pool_create()";

/* ********************************************************************** */
struct ap_net_conn_pool_t *ap_net_conn_pool_create(int max_connections, int connection_timeout_ms, int conn_buf_size,
                  ap_net_conn_callback_func accept_func, ap_net_conn_callback_func close_func, ap_net_conn_callback_func data_func,
                  ap_net_conn_callback_func destroy_func)
{
    struct ap_net_conn_pool_t *pool;


    ap_error_clear();

    pool = malloc(sizeof(struct ap_net_conn_pool_t));

    if ( pool == NULL )
    {
        ap_error_set(_func_name, AP_ERRNO_OOM);
        return NULL;
    }

    pool->max_connections = 0;
    pool->used_slots = 0;

    ap_net_conn_pool_set_max_connections(pool, max_connections, conn_buf_size);

    if ( ! ap_utils_timeval_set(&pool->max_conn_ttl, AP_UTILS_TIMEVAL_SET_FROMZERO, connection_timeout_ms) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "max_conn_ttl setup erro");
        free(pool);
        return NULL;
    }

    pool->accept_func = accept_func;
    pool->close_func = close_func;
    pool->data_func = data_func;
    pool->destroy_func = destroy_func;

    pool->listen_sock = 0;
    pool->poller = NULL;

    pool->stat.conn_count = 0;
    pool->stat.active_conn_count = 0;
    pool->stat.timedout = 0;
    pool->stat.queue_full_count = 0;
    pool->stat.total_time.tv_sec = 0;
    pool->stat.total_time.tv_usec = 0;

    return pool;
}
