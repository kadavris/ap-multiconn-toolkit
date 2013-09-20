/* part of AP's toolkit
 * ap_net/connpool/accept_connection
 */
#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"
#include <unistd.h>
#include <fcntl.h>

static const char *_func_name = "ap_net_accept_connection()";

/* ********************************************************************** */
/** \brief accepts new connection and adds it to the pool's list
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int
 *
 * returns NULL on error. connection pointer if OK.
 */
struct ap_net_connection_t *ap_net_conn_pool_accept_connection(struct ap_net_conn_pool_t *pool)
{
    int conn_idx;
    int n;
    int new_sock;
    struct ap_net_connection_t *conn;


    ap_error_clear();

    if (pool->used_slots == pool->max_connections)
    {
        ++pool->stat.queue_full_count;

        ap_error_set(_func_name, AP_ERRNO_CONNLIST_FULL);

        return NULL;
    }

    conn = NULL;

    for ( conn_idx = 0; conn_idx < pool->max_connections; ++conn_idx ) /*  finding free slot */
        if ( ! (pool->conns[conn_idx].state & AP_NET_ST_CONNECTED) )
        {
            conn = &pool->conns[conn_idx];
            break;
        }

    if ( conn == NULL )
        return NULL;

    if ( pool->type == AP_NET_CTYPE_TCP )
    {
        n = sizeof(conn->addr);

        new_sock = accept(pool->listen_sock, (struct sockaddr *)(&conn->addr), (socklen_t*)&n);

        if ( new_sock == -1 )
        {
            ap_error_set(_func_name, AP_ERRNO_SYSTEM);
            return NULL;
        }

        /* setting non-blocking connection.
           data exchange will be performed in the timer_event() called by alarm handler
           or by default by using poller
        */
        fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL) | O_NONBLOCK);

        conn->fd = new_sock;
    }
    else if ( pool->type == AP_NET_CTYPE_UDP )
    {
        conn->fd = -1;
    }

    gettimeofday(&conn->created_time, NULL);

    if ( timerisset( &pool->max_conn_ttl) )
    {
        timeradd(&conn->created_time, &pool->max_conn_ttl, &conn->expire);
    }
    else
    {
        timerclear( &conn->expire );
    }

    conn->bufptr = 0;
    conn->buffill = 0;

    conn->state = AP_NET_ST_CONNECTED; /* should be the sole bit set at this time */

    ++pool->used_slots;

    ++pool->stat.conn_count;
    pool->stat.active_conn_count += pool->used_slots;

    if (pool->accept_func != NULL)
        pool->accept_func(conn);

    if (ap_log_debug_level)
        ap_log_debug_log("* Got connected ([%d])\n", conn_idx);

    return conn;
}

