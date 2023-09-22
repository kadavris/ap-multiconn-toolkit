/** \file ap_net/conn_pool_poll.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Connections pool events poll procedures
 */
#include "conn_pool_internals.h"
#include "../ap_utils.h"
#include <time.h>
#include <unistd.h>

static const char *_func_name = "ap_net_conn_pool_poll()";

/** \brief Do poll task, adding, removing connection(s) and/or notifying user callback function of events
 *
 * \param pool struct ap_net_conn_pool_t*
 * \return int 1 if all OK, 0 if general error occurred
 *
 * Pretty much useless without callback function set in pool.
 * Closing disconnected sockets on graceful shut down by remote side (bit_is_set(conn->state, AP_NET_ST_DISCONNECTION))
 * Closing expired connections (conn->expire > 0)
 * Calling ap_net_conn_pool_accept_connection() on incoming from listener socket. Fires AP_NET_SIGNAL_CONN_ACCEPTED inside it
 * Calling ap_net_conn_pool_recv() on incoming data available at some connection. Fires AP_NET_SIGNAL_CONN_DATA_IN signal
 * Fires AP_NET_SIGNAL_CONN_CAN_SEND on pools with AP_NET_POOL_FLAGS_ASYNC flag set and socket is ready to send data
 *
 * Return 1 if all OK, 0 if general error occurred (see ap_error_get*())
 */
int ap_net_conn_pool_poll(struct ap_net_conn_pool_t *pool)
{
    int event_idx;
    int i;
    int n;
    struct epoll_event ev;
    struct ap_net_poll_t *poller;
    struct ap_net_connection_t *conn;
    char buf[100];


    ap_error_clear();

    poller = pool->poller;

    for (i = 0; i < pool->max_connections; ++i ) /* checking for zombies first */
    {
        conn = &pool->conns[i];

        if ( bit_is_set(conn->state, AP_NET_ST_DISCONNECTION) ) /* this state comes from previous poll cycle, so assuming the user did something before we drop it */
        {
            ap_net_conn_pool_close_connection(pool, i);
            continue;
        }
    }

    poller->events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);

    if (poller->events_count == -1)
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_SYSTEM, "epoll_wait()");
        return 0;
    }

    if( poller->debug && poller->events_count > 0 )
        ap_log_debug_log("---P-EVTCNT %d\n", poller->events_count);

    /* ==============================================================================================
     * first getting listener event
     */
    if ( pool->listener.sock != -1 )
    {
        for ( event_idx = 0; event_idx < poller->events_count; ++event_idx)
        {
            if ( poller->events[event_idx].data.fd != pool->listener.sock)
                continue;

            if ( bit_is_set(poller->events[event_idx].events, (EPOLLERR | EPOLLHUP)) ) /* connection's ERROR? */
            {
                ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "epoll reports error/hangup on listener socket");
                return 0;
            }

            conn = ap_net_conn_pool_accept_connection(pool); /* signal AP_NET_SIGNAL_CONN_ACCEPTED emitted from there */

            if ( conn == NULL )
            {
                if ( ap_error_get() == AP_ERRNO_ACCEPT_DENIED ) /* user denied. not an error */
                {
                    if( poller->debug )
                        ap_log_debug_log("\t-P-NOACCEPT - denied by callback\n");

                    break;
                }

                return 0;
            }

            if( poller->debug )
            {
                if ( bit_is_set(conn->flags, AP_NET_CONN_FLAGS_UDP_IN) )
                    ap_log_debug_log("\t-P-DataIn_UDP %d %s @ %d (p:%d f:%d s:%d)\n", conn->idx,
                            inet_ntop(conn->remote.af, (conn->remote.af == AF_INET ? 
                                (void*)&conn->remote.addr4.sin_addr : (void*)&conn->remote.addr6.sin6_addr), buf, 100),
                            ntohs(conn->remote.addr4.sin_port), conn->bufpos, conn->buffill, conn->bufsize);
                else
                    ap_log_debug_log("\t-P-ACCEPT %d\n", conn->idx);
            }

            break;
        }
    } /* if ( pool->listener.sock != -1 ) */

    /* ==============================================================================================
     * checking ordinary connections
     */
    for ( event_idx = 0; event_idx < poller->events_count; ++event_idx)
    {
         if ( poller->events[event_idx].data.fd == pool->listener.sock)
             continue;

         conn = ap_net_conn_pool_get_conn_by_fd(pool, poller->events[event_idx].data.fd);

         if ( conn == NULL || ! (bit_is_set(conn->state, AP_NET_ST_CONNECTED)) ) /* silently trying to free epoll of this missing connection's handle  */
         {
             ev.events = EPOLLIN;
             ev.data.fd = poller->events[event_idx].data.fd;

             epoll_ctl(poller->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev);

             if( poller->debug )
                 ap_log_debug_log("\t-P-FDERR\n");

             continue;
         }

         if ( bit_is_set(poller->events[event_idx].events, (EPOLLERR | EPOLLHUP)) ) /* connection's ERROR? */
         {
             conn->state |= AP_NET_ST_ERROR;

             ap_net_conn_pool_close_connection(pool, conn->idx);

             if( poller->debug ) ap_log_debug_log("\t-P-ERR %d\n", conn->idx);

             continue;
         }

         if ( bit_is_set(poller->events[event_idx].events, EPOLLIN) ) /*  data available for reading */
         {
              if( poller->debug)
                  ap_log_debug_log("\t-P-DATAIN %d(p:%d f:%d s:%d)", conn->idx, conn->bufpos, conn->buffill, conn->bufsize);

              n = ap_net_conn_pool_recv(pool, conn->idx);

              if ( n > 0 ) /* something new there */
              {
                  if( poller->debug)
                      ap_log_debug_log(" > (p:%d f:%d s:%d)\n", conn->bufpos, conn->buffill, conn->bufsize);

                  if ( pool->callback_func != NULL )
                      pool->callback_func(conn, AP_NET_SIGNAL_CONN_DATA_IN);
              }

              else if ( n == 0 ) /* ap_net_recv() returns this if there is no space buffer */
              {
                  if( poller->debug )
                      ap_log_debug_log(" -P- buffer full --\n");

                  /*
                  if ( conn->bufsize == 256 )
                  {
                      if( poller->debug)
                          ap_log_debug_log("DUMP: parent: %p, idx: %d, fd: %d\n\tladdr: %d/%d, raddr: %d/%d\n\tcre: %d.%ld, exp: %d.%ld\n\tbuf: %p, bs: %d, bp: %d, bf: %d\n\tstate: %d, ud :%p\n",
                                  conn->parent, conn->idx, conn->fd, conn->local.addr4.sin_addr.s_addr, conn->local.addr4.sin_port,
                                  conn->remote.addr4.sin_addr.s_addr,conn->remote.addr4.sin_port, conn->created_time.tv_sec, conn->created_time.tv_nsec,
                                  conn->expire.tv_sec, conn->expire.tv_nsec, conn->buf, conn->bufsize, conn->bufpos, conn->buffill, conn->state, conn->user_data
                          );

                  }
                  */
              }

              else if ( n == -2 ) /* ap_net_recv() returns this if connection is broken and user app should close it, but there can be some data left in buffer */
              {
                  if( poller->debug)
                      ap_log_debug_log("\t-P- Disconnect %d --\n", conn->idx);

                  /* freeing poller from wasting time. it will be removed on the next loop */
                  ap_net_conn_pool_poller_remove_conn(conn->parent, conn->idx);

                  conn->state |= AP_NET_ST_DISCONNECTION;
                  /* also setting expiration if none */
                  if ( ! ap_utils_timespec_is_set(&conn->expire) )
                      ap_utils_timespec_set(&conn->expire, AP_UTILS_TIME_SET_FROM_NOW, 2000);

                  if ( conn->buffill - conn->bufpos > 0 ) /* maybe user need the data left in buffer */
                  {
                      if ( pool->callback_func != NULL )
                           pool->callback_func(conn, AP_NET_SIGNAL_CONN_DATA_LEFT);
                  }

                  continue;
              }

              else /* some other error */
              {
                     if( poller->debug)
                          ap_log_debug_log("\t-P- ERROR %d --\n", conn->idx);

                  return 0;
              }
         } /* EPOLLIN */

         if ( bit_is_set(pool->flags, AP_NET_POOL_FLAGS_ASYNC) && bit_is_set(poller->events[event_idx].events, EPOLLOUT) ) /* can send data */
         {
              if ( pool->callback_func != NULL )
                   pool->callback_func(conn, AP_NET_SIGNAL_CONN_CAN_SEND);
         }
    } /*  for (event_idx = 0; event_idx < events_count */

    /* ==============================================================================================
     * now checking all connections for expiration etc.
     */
    for ( i = 0; i < pool->max_connections; ++i ) /* checking for expiration first */
    {
        conn = &pool->conns[i];

        if ( ! bit_is_set(conn->state, AP_NET_ST_CONNECTED) )
            continue;

        if ( ap_utils_timespec_is_set(&conn->expire) && 0 >= ap_utils_timespec_cmp_to_now( &conn->expire ) )
        {
            conn->state |= AP_NET_ST_EXPIRED;

            if ( pool->callback_func != NULL )
                 pool->callback_func(conn, AP_NET_SIGNAL_CONN_TIMED_OUT);

            ap_net_conn_pool_close_connection(pool, i);

            if( poller->debug )
                ap_log_debug_log("\t-PEXPIRED %d %ld ms\n", i, ap_utils_timespec_elapsed( &conn->expire, NULL, NULL ));
        }

        if ( poller->emit_old_data_signal && conn->buffill - conn->bufpos > 0 )
        {
            if ( pool->callback_func != NULL )
                 pool->callback_func(conn, AP_NET_SIGNAL_CONN_DATA_LEFT);
        }
    }

    return 1;
}

