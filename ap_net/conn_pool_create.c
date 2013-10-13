/* Part of AP's Toolkit
 * Networking module
 * ap_net/conn_pool_create.c
 */
#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_create()";

/* ********************************************************************** */
/** \brief Creates new connections pool record
 *
 * \param flags int - a bit set of AP_NET_POOL_FLAGS*
 * \param max_connections int - Maximum simultaneous connections the pool will handle
 * \param connection_timeout_ms int - default expiration time for new incoming connections. Milliseconds
 * \param conn_buf_size int - Incoming data buffer size for each of connection in pool
 * \param in_callback_func ap_net_conn_pool_callback_func - Callback function reference to perform signal-driven connection management
 * \return struct ap_net_conn_pool_t * - Pointer to the new pool record or NULL if error occurred
 *
 * The flags are affect pool type and behaviour. The flags can be combined by binary | (OR)
 * AP_NET_POOL_FLAGS_TCP - set pool to TCP mode. Absence of this flags means that pool created in UDP mode. No mixing on server! Probably on clients too.
 * 		Use different pools to provide different kinds of connections. This is only to simplify a set of functions to manage pool and individual connections.
 * AP_NET_POOL_FLAGS_IPV6 - set pool to IPv6 mode. Absence of this flag means IPv4 mode. No mixing on server! Client's pools are OK
 * AP_NET_POOL_FLAGS_ASYNC - trying to use fully asynchronous send/receive.
 * 		By default all connections sockets are set to non-blocking mode on creation. Polling for incoming data are asynchronous, but sending done with blocking flag.
 * 		Asynchronous mode turns on polling for 'can send' events on sockets and sends data via very smart function wrapped around non-blocking send.
 * 		See ap_net_conn_pool_send() for gory details.
 * 		As always, you can set by hand the blocking mode for each and other connection, but this will break poller and possible some other functions in unpredictable way.
 *
 * Max connections is really a count of connection record in pool's array. This could be resized almost any time by calling ap_net_conn_pool_set_max_connections()
 *
 * Connection timeout is set in milliseconds. it is used only for incoming connections. Outgoing timeout is set individually
 * After the expire time is come the connection is closed by ap_net_conn_pool_close_connection(). That is performed in ap_net_conn_pool_poll() and conn_pool_check*
 *
 * Connection buffer size set the length of each connection individual buffer for incoming data. Outgoing data buffer you must handle by self.
 * No zero length is allowed. set it to sane amount based on your needs.
 * The buffer can be (and meant to be) accessed directly by using connection->buf (char*), connection->bufsize for max length,
 * connection->buffill for data count, e.g. buf[buffill - 1] is the last byte of data, and connection->bufpos for current position in buffer.
 * The bufpos variable is mainly in user's hands and you must advance it properly at least to indicate that all data is processed if bufpos >= buffill.
 * This state is detected by receiving function in poller procedure and new data placed past the buffill marker if there is space left.
 * Or in special cases bufpos is reseted to zero, where case one is bufpos >= buffill - means that all data in buffer was processed and can be overwritten.
 * So new data will arrive from the beginning of buffer again.
 * Case two is when bufpos is past 2/3 of buffer size, the buffer is compacted, so data that left between the bufpos and buffill is mode to the buffer's begin
 * and bufpos and buffill are set to new values, possibly adding more arrived data.
 * By no means you must rely on some remembered char* pointer into the buffer. Always use relative indexes based on current value of bufpos.
 *
 * Callback function's coupled with ap_net_conn_pool_poll() main advantage is automatic handling of standard events like graceful and erroneous disconnections,
 * data arrival, registering new incoming connections is server mode and some changes to internal pool's structures such as moving connection from place to place
 * Current signals sent are:
 * 		AP_NET_SIGNAL_CONN_CREATED - Sent on each new connection structure create. This is happen on calling ap_net_conn_pool_set_max_connections() when new connections added
 * 			Useful to create and initialize connection's pointer to user-defined data connection->user_data
 *  	AP_NET_SIGNAL_CONN_DESTROYING - Called on connection's destruction.
 *  		1) also on ap_net_conn_pool_set_max_connections() when extra connections is removed
 *  		2) on pool's destruction
 *  	AP_NET_SIGNAL_CONN_CONNECTED - Called when new outgoing connection is ready to go. This is really for totally async feel.
 *  	AP_NET_SIGNAL_CONN_ACCEPTED - Called on new incoming connection is created and ready to be used.
 *  		This is special case. Callback function should return true if connection is allowed and false if it not desired.
 *  		In later case the connection is closed immediately and error is set to AP_ERRNO_ACCEPT_DENIED
 *  	AP_NET_SIGNAL_CONN_CLOSING - Called from ap_net_conn_pool_close_connection() _before_ the actual socket closing and structure data reset.
 *  		Let you do some last poking. Use carefully. Check connection->state for AP_NET_ST_ERROR, AP_NET_ST_CONNECTED, AP_NET_ST_EXPIRED, AP_NET_ST_DISCONNECTION
 *  		Look at ap_net.h for actual details on status bits
 *  	AP_NET_SIGNAL_CONN_MOVED_TO - Used in pair with AP_NET_SIGNAL_CONN_MOVED_FROM indicating the place the connection was copied/moved to
 *  	AP_NET_SIGNAL_CONN_MOVED_FROM - Used in pair with AP_NET_SIGNAL_CONN_MOVED_TO indicating the place the connection was copied/moved from
 *  	AP_NET_SIGNAL_CONN_DATA_IN - New data is available in receiving buffer
 *  	AP_NET_SIGNAL_CONN_CAN_SEND - Socket is ready for sending data. Used in asynchronous mode
 *  	AP_NET_SIGNAL_CONN_TIMED_OUT - Called on expiration event. Next signal will be AP_NET_SIGNAL_CONN_CLOSING
 *  	AP_NET_SIGNAL_CONN_DATA_LEFT - Funny companion to AP_NET_SIGNAL_CONN_DATA_IN. Called in poll cycle when no _new_ data was received,
 *  		but buffer still contain some unprocessed stuff. trigger is bufpos < buffill.
 *
 */
struct ap_net_conn_pool_t *ap_net_conn_pool_create(int flags, int max_connections, int connection_timeout_ms, int conn_buf_size, ap_net_conn_pool_callback_func in_callback_func)
{
    struct ap_net_conn_pool_t *pool;


    ap_error_clear();

    pool = malloc(sizeof(struct ap_net_conn_pool_t));

    if ( pool == NULL )
    {
        ap_error_set(_func_name, AP_ERRNO_OOM);
        return NULL;
    }

    pool->callback_func = in_callback_func;
    pool->max_connections = 0;
    pool->used_slots = 0;
    pool->conns = NULL;

    ap_net_conn_pool_set_max_connections(pool, max_connections, conn_buf_size);

    if ( ! ap_utils_timespec_set(&pool->max_conn_ttl, AP_UTILS_TIME_SET_FROMZERO, connection_timeout_ms) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_CUSTOM_MESSAGE, "max_conn_ttl setup");
        free(pool);
        return NULL;
    }

    memset(&pool->listener, 0, sizeof(pool->listener));

    pool->listener.sock = -1;
    pool->poller = NULL;

    pool->flags = flags;

    pool->stat.conn_count = 0;
    pool->stat.active_conn_count = 0;
    pool->stat.timedout = 0;
    pool->stat.queue_full_count = 0;
    pool->stat.total_time.tv_sec = 0;
    pool->stat.total_time.tv_nsec = 0;

    return pool;
}
