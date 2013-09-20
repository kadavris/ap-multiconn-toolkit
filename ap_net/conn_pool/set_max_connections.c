#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"
#include "../../ap_error/ap_error.h"

static const char *_func_name = "ap_net_conn_pool_set_max_connections()";

/* ********************************************************************** */
int ap_net_conn_pool_set_max_connections(struct ap_net_conn_pool_t *pool, int new_max, int new_bufsize)
{
    void *new_mem;
    int i;


    ap_error_clear();

    if ( pool->used_slots != 0 )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "can't resize - pool have open connections");
        return 0;
    }

    if ( new_max == pool->max_connections )
        return 1;

    if ( pool->max_connections == 0 )
    {
        pool->conns = malloc(pool->max_connections * sizeof(struct ap_net_connection_t));

        if( pool->conns == NULL )
        {
             ap_error_set(_func_name, AP_ERRNO_OOM);
             return 0;
        }
    }
    else /*  resizing */
    {
        if ( new_max < pool->max_connections )
        {
            for ( i = new_max; i < pool->max_connections; ++i) /*  removing extra */
            {
                free(pool->conns[i].buf);
            }
        }

        new_mem = realloc(pool->conns, new_max * sizeof(struct ap_net_connection_t));

        if ( new_mem == NULL )
        {
        	 ap_error_set(_func_name, AP_ERRNO_OOM);
             return 0;
        }

        pool->conns = new_mem;
    }

    if ( new_max > pool->max_connections )
    {
        for ( i = pool->max_connections; i < new_max; ++i)
        {
            pool->conns[i].fd = 0;
            pool->conns[i].idx = i;
            pool->conns[i].parent = pool;

            pool->conns[i].state = 0;
            pool->conns[i].user_data = NULL;

            pool->conns[i].bufptr = 0;
            pool->conns[i].buffill = 0;
            pool->conns[i].bufsize = new_bufsize;
            pool->conns[i].buf = malloc(new_bufsize);

            if ( pool->conns[i].buf == NULL)
            {
            	ap_error_set(_func_name, AP_ERRNO_OOM);
                return 0;
            }
        }
/*TODO: recreate poller */
    }

    pool->max_connections = new_max;

    return 1;
}

