/* Part of AP's Toolkit
 * ap_net.h
 * main include file for networking functions
 */
#ifndef AP_NET_H
#define AP_NET_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

/* connection statuses bits */
	    /* ERROR state. Without doubt you should not perform i/o operations on this connection anymore */
#define AP_NET_ST_ERROR          1
		/* Connection is up. Practically means that this record is used and possibly connected, but you must check for other bits too */
#define AP_NET_ST_CONNECTED      2
		/* Busy on internal operation. Should not be visible outside of toolkit. */
#define AP_NET_ST_BUSY           4
		/* Performing data input. Should not be visible outside of toolkit. */
#define AP_NET_ST_IN             8
		/* Performing data output. Should not be visible outside of toolkit. */
#define AP_NET_ST_OUT           16
		/* Expired, close pending. Set in poller, on call to ap_net_conn_pool_close_connection() */
#define AP_NET_ST_EXPIRED       32
		/* Closing. Set in poller when data input attempt returns that peer disconnected gracefully. ap_net_conn_pool_poll() closes those on start, so do your best */
#define AP_NET_ST_DISCONNECTION 64

/* Flags for pools */
		/* Pool is of TCP type. Absence of this flag means UDP pool */
#define AP_NET_POOL_FLAGS_TCP    1
		/* Perform fully asynchronous I/O operations */
#define AP_NET_POOL_FLAGS_ASYNC  2
		/* Pool is in IPv6 mode. Absence of this flag means IPv4 mode. Have sense in server mode */
#define AP_NET_POOL_FLAGS_IPV6   4

/* Flags for connections */
		/* for incoming UDP connections we should read input on listener socket instead */
#define AP_NET_CONN_FLAGS_UDP_IN  1

/* return bits of ap_net_poller_* */
		/* Status returned for listener socket */
#define AP_NET_POLLER_ST_LISTENER 1
		/* error detected */
#define AP_NET_POLLER_ST_ERROR    2
		/* data available from peer */
#define AP_NET_POLLER_ST_IN       4
		/* Socket ready to send data */
#define AP_NET_POLLER_ST_OUT      8

/* signals for callback function. see ap_net_conn_pool_create() for detailed description */
#define AP_NET_SIGNAL_CONN_CREATED     0
#define AP_NET_SIGNAL_CONN_DESTROYING  1
#define AP_NET_SIGNAL_CONN_CONNECTED   2
#define AP_NET_SIGNAL_CONN_ACCEPTED    3
#define AP_NET_SIGNAL_CONN_CLOSING     4
#define AP_NET_SIGNAL_CONN_MOVED_TO    5
#define AP_NET_SIGNAL_CONN_MOVED_FROM  6
#define AP_NET_SIGNAL_CONN_DATA_IN     7
#define AP_NET_SIGNAL_CONN_CAN_SEND    8
#define AP_NET_SIGNAL_CONN_TIMED_OUT   9
#define AP_NET_SIGNAL_CONN_DATA_LEFT  10

typedef struct ap_net_conn_pool_t ap_net_conn_pool_t;

/* ********************************************************************** */
typedef struct ap_net_connection_t
{
    struct ap_net_conn_pool_t *parent; /**< pointer to parent pool if any */
    int idx; /**< index in parent pool array */
    int fd; /**< socket file descriptor  */
    unsigned flags; /* AP_NET_CONN_FLAGS_* */

    union  /* local address. filled on connect() */
    {
        sa_family_t af; /* address family: AF_INET*. */
    	struct sockaddr_in  addr4;
    	struct sockaddr_in6 addr6;
    } local;

    union /**< far side of the connection */
    {
        sa_family_t af; /* address family: AF_INET*. */
        struct sockaddr_in  addr4;
        struct sockaddr_in6 addr6;
    } remote;

    struct timespec created_time;
    struct timespec expire;

    char *buf; /**< Incoming data buffer. */
    int bufsize, bufpos, buffill; /**< buf size/current pos + filled bytes count */

    unsigned state; /**< AP_NET_ST_* */

    void *user_data; /**< user-defined data storage */
} ap_net_connection_t;

/* ********************************************************************** */
typedef struct ap_net_poll_t
{
    struct ap_net_conn_pool_t *parent; /* can be NULL for stand alone */
    struct epoll_event *events;
    int max_events;
    int last_event_index; /* used for stand alone poller, cyclically returning the next event from ap_net_poller_poll() */
    int events_count; /* used for stand alone poller, current events count */
    int epoll_fd;
    int listen_socket_fd;
    int listen_socket_flags; /* AP_NET_POOL_FLAGS_* */
    int debug; /* if true will output polling events via ap_log_debug_log */
    int emit_old_data_signal; /* if true (default) then poller will emit AP_NET_SIGNAL_CONN_DATA_LEFT signals if connection receiving buffer is already filled with data */
} ap_net_poll_t;

/* ********************************************************************** */
typedef struct ap_net_stat_t
{
    unsigned conn_count; /* all time connections count */
    unsigned timedout;   /* how many timed out */
    unsigned queue_full_count;  /* how many was dropped because of queue full */
    unsigned active_conn_count; /* a sum of active connections for the each new created. use for avg_conn_count = active_conn_count / conn_count */
    struct timespec total_time;  /* total time for all closed connections */
} ap_net_stat_t;

typedef int (*ap_net_conn_pool_callback_func)(struct ap_net_connection_t *conn, int signal_type); /* signal_type is of AP_NET_SIGNAL_* */

/* ********************************************************************** */
typedef struct ap_net_conn_pool_t
{
    struct ap_net_connection_t *conns; /* array */
    int max_connections; /* connections array size */
    int used_slots; /* count of used slots in conns array */
    unsigned flags; /* AP_NET_POOL_FLAGS_* */
    unsigned state; /* AP_NET_ST_* */

    struct ap_net_poll_t *poller;

    struct timespec max_conn_ttl;   /*  max connection stall time. force closed after that */

    struct
    {
    	int sock; /* < 0 if no bind was made */
    	union /* local address. filled on connect() */
    	{
    		struct sockaddr_in  addr4;
    		struct sockaddr_in6 addr6;
    	};
    } listener;

    ap_net_conn_pool_callback_func callback_func;

    struct ap_net_stat_t stat;
} ap_net_conn_pool_t;

/* ********************************************************************** */
extern struct ap_net_conn_pool_t *ap_net_conn_pool_create(int flags, int max_connections, int connection_timeout_ms, int conn_buf_size, ap_net_conn_pool_callback_func in_callback_func);

 /*  initializes connection structure with default sane values */
extern void ap_net_conn_pool_connection_pre_connect(struct ap_net_conn_pool_t *pool, int conn_idx, unsigned flags);

 /* fills address info used for bind and/or outgoing connections */
extern int ap_net_conn_pool_set_str_addr(int af, void *destination, const char *address_str, socklen_t addr_len, int port);
extern int ap_net_conn_pool_set_ip4_addr(struct sockaddr_in *destination, in_addr_t address4, int port);
extern int ap_net_conn_pool_set_ip6_addr(struct sockaddr_in6 *destination, struct in6_addr *address6, int port);

/*  returns binded socket. -1 on any error. */
extern int ap_net_conn_pool_listener_create(struct ap_net_conn_pool_t *pool, int max_tries, int retry_sleep);
 /*  accepts new connection and adds it to the list. you must do ap_net_conn_pool_bind() first or initialize pool->listen_sock manually */
extern struct ap_net_connection_t *ap_net_conn_pool_accept_connection(struct ap_net_conn_pool_t *pool);

extern struct ap_net_connection_t *ap_net_conn_pool_connect_straddr(struct ap_net_conn_pool_t *pool, unsigned flags, const char *address_str, int af, int port, int expire_in_ms);
extern struct ap_net_connection_t *ap_net_conn_pool_connect_ip4(struct ap_net_conn_pool_t *pool, unsigned flags, in_addr_t address4, int port, int expire_in_ms);
extern struct ap_net_connection_t *ap_net_conn_pool_connect_ip6(struct ap_net_conn_pool_t *pool, unsigned flags, struct in6_addr *address6, int port, int expire_in_ms);

 /*  close connection by index */
extern void ap_net_conn_pool_close_connection(struct ap_net_conn_pool_t *pool, int conn_idx);

 /*  manual check for dropped connections */
extern int  ap_net_conn_pool_check_conns(struct ap_net_conn_pool_t *pool);
extern int  ap_net_conn_pool_connection_is_alive(struct ap_net_conn_pool_t *pool, int conn_idx); /*  returns true if alive */

extern int  ap_net_conn_pool_recv(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int  ap_net_conn_pool_send(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size);
extern int  ap_net_conn_pool_send_async(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size);

extern void ap_net_conn_pool_print_stat(struct ap_net_conn_pool_t *pool, char *intro_message); /*  print stats to debug channel */

extern int  ap_net_conn_pool_set_max_connections(struct ap_net_conn_pool_t *pool, int new_max, int new_bufsize);

extern void ap_net_connection_copy(struct ap_net_connection_t *dst_conn, struct ap_net_connection_t *src_conn);
extern int  ap_net_conn_pool_move_conn(struct ap_net_conn_pool_t *dst_pool, struct ap_net_conn_pool_t *src_pool, int conn_idx);

extern void ap_net_connection_destroy(struct ap_net_connection_t *conn, int free_this);
extern void ap_net_conn_pool_destroy(struct ap_net_conn_pool_t *pool, int free_this);

extern int  ap_net_connection_lock(struct ap_net_connection_t *conn);
extern void ap_net_connection_unlock(struct ap_net_connection_t *conn);
extern int  ap_net_conn_pool_lock(struct ap_net_conn_pool_t *pool);
extern void ap_net_conn_pool_unlock(struct ap_net_conn_pool_t *pool);

extern struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_fd(struct ap_net_conn_pool_t *pool, int fd);
extern struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_port(struct ap_net_conn_pool_t *pool, in_port_t port, int is_local);
extern struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_address(struct ap_net_conn_pool_t *pool, struct sockaddr_storage *ss, int is_local);
extern struct ap_net_connection_t *ap_net_conn_pool_find_free_slot(struct ap_net_conn_pool_t *pool);

/*  stand alone poller functions: */
extern struct ap_net_poll_t *ap_net_poller_create(int listen_socket_fd, int max_connections);
extern int  ap_net_poller_poll(struct ap_net_poll_t *poller);
extern int  ap_net_poller_single_fd(int fd);
extern void ap_net_poller_destroy(struct ap_net_poll_t *poller);

/*  conn_pool poller functions: */
extern int  ap_net_conn_pool_poll(struct ap_net_conn_pool_t *pool);
extern int  ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool);

extern void ap_net_connection_buf_clear(struct ap_net_connection_t *conn, int fill_char);

#endif
