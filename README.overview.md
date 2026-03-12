# This guide is a quick introduction to **AP's Multiconn Toolkit**, a C library for client-server communication management via TCP and UDP.

and is for the V1.0 of the library.

There are several sets of modules, that mostly interconnect with each other:

- [ap_net.h - networking functions. Contains TCP/UDP connections pool and epoll()-based helpers](#ap_neth---connections-pool)
- [ap_log.h - logging and debugging facilities. Contains log messages broadcasting tools. Error control for toolkit](#ap_logh---logging-and-debugging)
- [ap_str.h - strings manipulation. Mainly `strtok()` wrapper](#ap_strh---string-manipulation)
- [ap_utils.h - Miscellaneous utility functions that dont fall into one of above categories.](#ap_utilsh---miscellaneous-utilities)

## ap_net.h - Connections pool

This module provides one big system: non-threaded connection pool - a management suite for client-server communication via TCP or UDP. 
Almost all of the functions can be used directly, controlling pool's connection list with manual `accept()`, `receive()`, `send()`, `close()`.  
But intended usage is a kind of asynchronous, where you set up a pool and then cycle the polling function. That will take the burden of calling secondary service procedures such as `accept`, `recv` or `close` and notifying your **callback function** of pending events.  
The **callback function** will receive signals for all kinds of socket activity that may require special attention. This includes new connection or data arrival, connection errors and expiration, etc.  
  
That said, the server-side usage pattern where we will want a listener socket is to:

1) create a pool
2) create an internal listener in this pool
3) `while ( something ) do_poll()`

The client-side usage is the same, but instead of a listener we call `connect()` between polls.
  
For simplicity reasons, each server-side pool can have only one TCP or UDP listener port. Also, the server's pool operating IP type is limited to either IPv4 or IPv6 - which is set in pool's flags. To do both you'll need two pools.

The client's pool, being only for the outgoing connections, ignores those restrictions.

Of course you can have as many pools of any kind as you wish. The good news is you can have a single callback function for all of them, so there is really no big difference in managing one or many pools.  
  
## Creating the pool

```C
struct ap_net_conn_pool_t *pool;  
...
pool = ap_net_conn_pool_create(POOL_FLAGS, MAX_CONNECTIONS_SLOTS, DEFAULT_TIMEOUT_IN_MS, CONNECTION_RECEIVING_BUFFER_SIZE, callback_function);
```

Where:

- `POOL_FLAGS` bit-field controlling specifics of this pool instance behavior, like is it for TCP or UDP, IPv4 or IPv6 modes, etc (see `AP_NET_POOL_FLAGS_*`)

- `MAX_CONNECTIONS_SLOTS` – is the hard limit of possible simultaneous connections.  
You can lift or drop this number later via call to `ap_net_conn_pool_set_max_connections()`

- `DEFAULT_TIMEOUT_IN_MS` – for incoming connections there will be default expiration time set from this parameter value in *milliseconds*. Expired connections will be closed automatically in polling function. Callback function will be notified *before* closing

- `CONNECTION_RECEIVING_BUFFER_SIZE` – each connection in a pool has an individual buffer for incoming data. This number sets how many bytes it will hold.  
Set it thoughtfully. The tricky part is you are required to behave nicely with this buffer and obey some simple rules to get flawless data stream.

- `callback_function` - is your application's callback that gets two parameters: a connection's structure pointer and the signal type. It will receive all notifications about what is happening with the pool(s) it is assigned to.

```C
int callback_func(struct ap_net_connection_t *conn, int signal_type)
```

`signal_type` is one of `AP_NET_SIGNAL_*`  
The return value of this function should be true/false for user acknowledgement to accept a new connection and ignored in most other cases.

## Creating the listener

```C
if ( ! ap_net_conn_pool_set_str_addr(pool, (char *)BIND_ADDRESS_STRING, LISTENER_PORT)  
    || -1 == ap_net_conn_pool_listener_create(pool, MAX_TRIES, RETRY_SLEEP)  
    )  
    report_error();
```

Where:

- `ap_net_conn_pool_set_str_addr()` - sets the listener's bind IP address from a string address form and integer port.

- `ap_net_conn_pool_listener_create()` - binds and issues `listen()` calls.

- The `MAX_TRIES` parameter sets a count of retry attempts on `bind()` call.
This can be useful on a system start-up, when a daemon or a service can be loaded prior to networking interface initialization. Although for an OS like Linux it is better to use systemd load ordering.
The companion `RETRY_SLEEP` parameter sets the sleep() seconds count between tries.

## Processing connections

Now we have ready to process incoming connections pool.  
And by running something like:

```C
for(;;)  
{  
    if ( ! ap_net_conn_pool_poll( pool ) )  
        report_error();  
}
```

That's nearly all that's neededed.

Don't forget the callback:

```C
int my_callback ( struct ap_net_connection_t *conn, int signal_type )
{
    switch ( signal_type )
    {
        case AP_NET_SIGNAL_CONN_CREATED:
            /* A new connection _structure_ have been created.
            This is not an actual network connection event.
            Time to attach your own data if any to the connection's hook */
            allocate_connection_user_data( conn );
            break;

        case AP_NET_SIGNAL_CONN_DESTROYING:
            /* Connection is pending for removal.
            Can be called on the pool's array downsize.
            Time to unhook your data bound with this connection */
            free_connections_user_data( conn );
            break;

        case AP_NET_SIGNAL_CONN_CONNECTED:  
            /* This is for the outgoing connection attempt made successful */
            send_first_packet_as_client( conn );
            break;  

        case AP_NET_SIGNAL_CONN_ACCEPTED:  
            /* And this is a new, incoming connection established and waiting our approval */
            if ( ! check_if_we_want_this_client_at_all( conn ) )
                return 0; /* dont' want it */
            initialize_connection_user_data(conn);
            return 1; /* approved */

        case AP_NET_SIGNAL_CONN_CLOSING:
            /* Either closing by a manual request, or peer's graceful shutdown,
            or any other event, or fatal error.
            Maybe it's time to unhook your data too */
            stop_data_processing_for_connection( conn );  
            break;

        case AP_NET_SIGNAL_CONN_DATA_IN:  
            /* Some bytes arrived. You may do something about it */
            process_incoming_data( conn );
            break;

        case AP_NET_SIGNAL_CONN_TIMED_OUT:  
            /* If expiration is set to a non-zero number of milliseconds during the pool creation
            or manually corrected in the connection itself, then this signal
            will arrive when it's less than current clock (i.e., connection time limit exceeded) */
            notify_of_a_slowpoke_client( conn );
            /* The next and the last signal for this connection will be AP_NET_SIGNAL_CONN_CLOSING */
            break;  
    } /* end of switch */  
} /* end of callback() */
```

Some important notes:

- The mentioned 'hook' in connection's structure is a pointer to the set of your application's data you want to be immediately available for any of callback processing. On connection structure creation it is initialized to NULL, so you can easily distinguish between hooked and not hooked.

- The accepting procedure closes connection immediately if after emitting `AP_NET_SIGNAL_CONN_ACCEPTED` signal, the callback returns `false`, as seen in the above example `switch()`

- Default expiration for incoming connections can be set in the `ap_net_conn_pool_create()` call.  
The actual expiration time is corrected on accept and connect events to be current time + the pool's expiration. If default is set to 0, no expiration is performed automatically.  
**Beware of stalled connections!** You can use the `ap_utils_timespec_` function set to manipulate `conn->expire` value manually.

- Check for error and other states in a bit-field `conn->state`. See `AP_NET_ST_` in `ap_net.h` for the description.

### Controlling how much data is there

Upon receiving an event for the incoming data, `ap_net_conn_pool_poll()` automatically calls the `ap_net_conn_pool_recv()` function that tries to receive data into this connection's individual buffer.  
Upon arrival of some bytes, they are placed starting from `conn->buf + conn->buffill` position if there is some space left.  
After that the count of bytes stored is added to the `conn->buffill`.  
That is your high watermark. The low watermark is `conn->bufpos`.  
It is mandatory that you update this marker after processing a portion of data. Its state is monitored and if it is past `conn->buffill`, then the buffer is considered 'exhausted' and new data will arrive from the start of it. The `bufpos` value is set to zero then.  
So the main rule is not to use some direct-to-buffer `char *` pointers as the data can be moved or replaced within cyclic poll() calls.

Always check the connection's `bufpos` and `buffill` fields. Also, you can force the remaining buffer data to be discarded by setting `buffill` to zero.

**And that's basically the core of the process, that will allow you to start.**

For the bulk and details you may want to check `ap_net/ap_net.tests.c`.
It is a comprehensive and somewhat ugly example of how to create fully functional client and server.

## ap_log.h - logging and debugging

This facility is light and clean.

First, there are two global variables that are used to control the output:
    - `int ap_log_debug_to_tty` - Boolean flag telling whether to output debug messages to stderr along with other registered debug channel(s)
    - `int ap_log_debug_level` - Verbosity of debug messages.

Second, you should register file or socket descriptor to be used as the debug messages output channel.  
Multiple channels are allowed at once. That way your messages can go to the console, log file and to the pair of remote staff members that used to telnet to your software's debugging IP port.

The `*debug_handle*` functions group is used to add or remove file or socket descriptors from the debug output channels list:

- `int ap_log_add_debug_handle( int fd )` - registers file/socket handle for debug output

- `int ap_log_remove_debug_handle( int fd )` - removes handle from debug output targets list

- `int ap_log_is_debug_handle( int fd )` - checks if handle is registered for debug output

And third, you can start writing your messages with:

- `ap_log_debug_log(char *fmt, ...)` - printf()-style logs something to the debug channel(s)

- `ap_log_debug_log_output( char *buf, int buflen )` - writes the pure buf content into debugging channel(s)

Additionally, there is a common syslog facility wrapper:

- `void ap_log_do_syslog(int priority, char *fmt, ...)` that puts your message to syslog and also calls `ap_log_debug_log()` if `ap_log_debug_level` is > 0

In case an error occurrs inside of a toolkit's function, you can get its number by calling `int ap_error_get( void )`, with a code of 0 meaning no error (see `ap_error/*.h` for a list of `AP_ERROR` definitions.  
The complementary function is `const char *ap_error_get_string( void )` that prepares a human-readable message with information on the last error that occurred.  
The message will have the function name where error was caught, definition of error and system errno + strerror() call result if there was actually a problem with some system call.

The `ap_log_h` functions set is designed to be much like fprintf/fputs/fputc standard calls, but instead of a FILE* argument type it works with numeric file or socket descriptor.

- `int ap_log_hprintf(int fh, char *fmt, ...)` - like `fprintf()`

- `int ap_log_hputs(char *str, int fh)` - like `fputs()`

- `int ap_log_hputc(char c, int fh)` - like `fputc()`

And the `ap_log_mem_` functions group is for raw memory area byte values display as a dump:

- `void ap_log_mem_dump_to_fd(int fh, void *p, int len)` - hexadecimal + printable characters dump into a file handle

- `void ap_log_mem_dump(void *p, int len)` - hexadecimal + printable characters dump to the (registered) debug channel(s)

- `void ap_log_mem_dump_bits_to_fd(int fh, void *p, int len)` - bits values dump into specified file handle

- `void ap_log_mem_dump_bits(void *p, int len)` - bit values dump into debug channel(s)

## ap_str.h - string manipulation

The main functions set is `ap_str_parse*` which is a wrapper around strtok(), plus some additional features:

- `ap_str_parse_rec_t *ap_str_parse_init(char *in_str, char *user_separators)` - creates a new parsing record from the `in_str`, using characters from the `user_separators` if parameter is not NULL, otherwise 'space' and 'tab' characters as a default.  
The `in_str` is copied into the parsing record, so you don't have to keep it intact for subsequent parsing.

- `void ap_str_parse_end(ap_str_parse_rec_t *)` - cleans up the parse record, freeing internal memory allocations

- `char *ap_str_parse_next_arg(ap_str_parse_rec_t *)` - returns the next token or a NULL if end of the string encountered

- `char *ap_str_parse_get_remaining(ap_str_parse_rec_t *)` - gets the remnants of string without parsing it further

- `char *ap_str_parse_rollback(ap_str_parse_rec_t *r, int roll_count)` - rolls n tokens back, restoring unparsed string. Reparses string at the requested point and returns a pointer to a token.

- `char *ap_str_parse_skip(ap_str_parse_rec_t *r, int skip_count)` - skips n tokens forward. Returns a ptr to a token after these.

- `int ap_str_parse_get_bool(ap_str_parse_rec_t *)` - treats next token as a string representation of a boolean value and returns it. -1 in case of error. Recognized word pairs are on/off, 1/0, true/false, enable/disable  

## Other string functions

- `void *ap_str_getmem(int size, char *errmsg)` - A `malloc` with error checking. In case of error, if `errmsg != NULL` it is reported via `ap_log_do_syslog()` and calls `exit(1)` at the end

- `int ap_str_makestr(char **destination, const char *source)` - `strdup()`-like, but with auto `free/malloc` calls for the destination. If the  destination ptr is NULL, then it is being allocated and data from the source is copied. If it is not NULL, then it is `free()`-d first, then allocation and copy is performed.

- `int ap_str_fix_buf_size(char **buf, int *bufsize, int *bufpos, int needbytes)` - checks if some buffer can hold additional data of specified size and if not, the `realloc()` is called and `bufsize` is updated accordingly

- `int ap_str_put_to_buf(char **buf, int *bufsize, int *bufpos, void *src, int srclen)` - puts data into the buffer, calling `ap_str_fix_buf_size()` first, to check and enlarge if needed, then `srclen` bytes are copied into `buf`, starting with `bufpos` offset  

## ap_utils.h - Miscellaneous utilities

What we have here is a set of time-related functions dealing with `struct timeval` and `struct timespec`

### `struct timeval` functions

- `int ap_utils_timeval_cmp_to_now(struct timeval *)` - performs a comparison of the timeval versus the current clock, returning -1 if timeval is less than, 0 if == now and 1 if it is past the current time

- `int ap_utils_timeval_set(struct timeval *tv, int mode, int msec)` - corrects or sets `tv` value depending on `mode`, using `msec` as a milliseconds count or offset. The modes are:

  - `AP_UTILS_TIME_ADD` - adds `msec` amount to `*tv`'s current value

  - `AP_UTILS_TIME_SUB` - subtracts `msec` amount from `*tv`'s current value

  - `AP_UTILS_TIME_SET_FROM_NOW` - sets `*tv` value to current time + `msec`

  - `AP_UTILS_TIME_SET_FROMZERO` - sets value exactly to `msec` milliseconds

### `struct timespec` functions

These are used to hold time values inside the `ap_net` module(s)

- `int ap_utils_timespec_cmp_to_now(struct timespec *ts)` - performs a comparison of the `*ts` value versus the current clock, returning -1 if `*tv` is less than, 0 if == now and 1 if past current time

- `void ap_utils_timespec_clear(struct timespec *ts)` - sets `*ts` value to zero

- `int ap_utils_timespec_set(struct timespec *ts, int mode, int msec)` - same as `ap_utils_timeval_set()` (see above)

- `int ap_utils_timespec_is_set(struct timespec *ts)` - returns true if `*ts` is not zero

- `void ap_utils_timespec_add(struct timespec *a, struct timespec *b, struct timespec *destination)` - calculates `destination` = `a` plus `b`

- `void ap_utils_timespec_sub(struct timespec *a, struct timespec *b, struct timespec *destination)` - calculates `destination` = `a` minus `b`

- `long ap_utils_timespec_elapsed(struct timespec *begin, struct timespec *end, struct timespec *destination)` - counts elapsed time: `destination` = `end` - `begin`, returns a difference in milliseconds.  
`begin` **or** `end` can be `NULL`, in which case one is replaced by the current clock value.  
For example if the `end` is `NULL` then we get `destination` = `now` - `begin`.  
`destination` can be `NULL` too if you only need a returned milliseconds value.

- `long ap_utils_timespec_to_milliseconds(struct timespec *ts)` - Returns `*ts` value as milliseconds

### Other functions and features

- `uint16_t count_crc16(void *mem, int len)` - does a standard CRC16 calculation of a memory area of length `len`

- bit_get(bit_field, bit_number), bit_is_set(bit_field, bit_number), bit_set(bit_field, bit_number), bit_clear(bit_field, bit_number), bit_flip(bit_field, bit_number), bit_write(set_it, bit_field, bit_number), BIT(bit_number) -  
are macros for bit-fields manipulation

AP's Multiconn Toolkit is Copyright 2013+ by Andrej Pakhutin (kadavris\<at>gmail.com)
