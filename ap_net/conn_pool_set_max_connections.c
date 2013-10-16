/** \file ap_net/conn_pool_set_max_connections.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Setting or changing pool maximum connections amount for pool
 */
#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_set_max_connections()";

/* ********************************************************************** */
/** \brief send data from user's buffer with blocking
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param new_max int - new maximum connections count
 * \param new_bufsize int - receiving buffer size for newly added connections
 * \return int - true/false
 *
 * Enlarge or shrink the list of connections slots in pool.
 * You cannot shrink it less that the count of active connections. This is returned as AP_ERRNO_CONNLIST_FULL error.
 * On downsizing process the active connections can be moved closer to the beginning of list to free the tail space that will be removed
 * Signals AP_NET_SIGNAL_CONN_MOVED_TO and AP_NET_SIGNAL_CONN_MOVED_FROM are emitted in process to let user change connection->user_data accordingly.
 * Also AP_NET_SIGNAL_CONN_CREATED signal is emitted for each newly added connection slot, AP_NET_SIGNAL_CONN_DESTROYING for each removed
 * This function is called inside the ap_net_conn_pool_create() to set up the initial connections list
 */
int ap_net_conn_pool_set_max_connections(struct ap_net_conn_pool_t *pool, int new_max, int new_bufsize)
{
    void *new_mem;
    int i;
    int n;
    int retval;


    ap_error_clear();

    if ( new_max == pool->max_connections )
        return 1;

	if ( new_max < pool->used_slots ) /* no free slots for live connections - no downsizing */
	{
		ap_error_set_detailed(_func_name, AP_ERRNO_CONNLIST_FULL, "Can't downsize pool: too many live connections");
		return 0;
	}

    if ( ! ap_net_conn_pool_lock(pool))
    {
        ap_error_set(_func_name, AP_ERRNO_LOCKED);
    	return 0;
    }

    for ( i = 0; i < pool->max_connections; ++i ) /* locking conns */
    	if ( ! ap_net_connection_lock(&pool->conns[i]))
    	{
    		while(--i) /* undo on previous */
    			ap_net_connection_unlock(&pool->conns[i]);

            ap_net_conn_pool_unlock(pool);
            ap_error_set(_func_name, AP_ERRNO_LOCKED);
    		return 0;
    	}

    retval = 0; /* error state by default */

    if ( new_max < pool->max_connections )
    {
        n = -1;

        /* defragmenting. moving active connections from end to free slots at the beginning of connections list */
        for ( i = new_max - 1; i < pool->max_connections; ++i )
        {
        	for (++n;; ++n ) /* finding free slot */
        		if ( ! (pool->conns[n].state & AP_NET_ST_CONNECTED) )
        			break;

            ap_net_connection_copy(&pool->conns[n], &pool->conns[i]);
            bit_clear(pool->conns[i].state, AP_NET_ST_CONNECTED);
            pool->conns[i].fd = -1;

            if ( pool->callback_func != NULL )
   			{
        		pool->callback_func(&pool->conns[n], AP_NET_SIGNAL_CONN_MOVED_TO);
        		pool->callback_func(&pool->conns[i], AP_NET_SIGNAL_CONN_MOVED_FROM);
            }
        }

        for ( i = new_max; i < pool->max_connections; ++i ) /* destroying extra */
        	ap_net_connection_destroy(&pool->conns[i], 0);
    } /* if ( new_max < pool->max_connections ) */

	new_mem = realloc(pool->conns, new_max * sizeof(struct ap_net_connection_t));

    if ( new_mem == NULL )
    {
     	 ap_error_set(_func_name, AP_ERRNO_OOM);
         goto unlock;
    }

    pool->conns = new_mem;

    if ( new_max > pool->max_connections )
    {
        for ( i = pool->max_connections; i < new_max; ++i)
        {
            pool->conns[i].fd = -1;
            pool->conns[i].idx = i;
            pool->conns[i].parent = pool;

            pool->conns[i].state = 0;

            pool->conns[i].bufpos = 0;
            pool->conns[i].buffill = 0;
            pool->conns[i].bufsize = new_bufsize;
            pool->conns[i].buf = malloc(new_bufsize);

            if ( pool->conns[i].buf == NULL )
            {
                pool->conns[i].bufsize = 0;
                ap_error_set(_func_name, AP_ERRNO_OOM);
                goto unlock;
            }

            pool->conns[i].user_data = NULL;

            if ( pool->callback_func != NULL )
        		pool->callback_func(&pool->conns[i], AP_NET_SIGNAL_CONN_CREATED);

        }
    }

    pool->max_connections = new_max;
    retval = 1;

unlock:
	for (n = 0; n < pool->max_connections; ++n )
		ap_net_connection_unlock(&pool->conns[n]);

	ap_net_conn_pool_unlock(pool);

    return retval;
}
