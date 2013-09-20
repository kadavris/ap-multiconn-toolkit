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
#define AP_NET_ST_ERROR       1
#define AP_NET_ST_CONNECTED   2
#define AP_NET_ST_BUSY        4
#define AP_NET_ST_IN          8
#define AP_NET_ST_OUT        16

/* connection type */
#define AP_NET_CTYPE_TCP  0
#define AP_NET_CTYPE_UDP  1


typedef struct ap_net_conn_pool_t ap_net_conn_pool_t;

/* ********************************************************************** */
typedef struct ap_net_connection_t
{
    struct ap_net_conn_pool_t *parent;
    int idx; /*  index in pool's array */

    int type; /*  AP_NET_CTYPE_* */
    int fd; /*  file descriptor (-1 = udp, etc. check 'type') */
    struct sockaddr_in addr; /*  who's connected to */
    struct timeval created_time;
    struct timeval expire;

    char *buf; /*  Incoming data buffer. */
    int bufsize, bufptr, buffill; /*  buf size/current pos + filled bytes count */

    unsigned state; /*  AP_NET_ST_* */

    void *user_data; /*  user-defined data storage */
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
    int listen_socket_type; /* AP_NET_CTYPE_* */
} ap_net_poll_t;

/* ********************************************************************** */
typedef struct ap_net_stat_t
{
    unsigned conn_count; /* all time connections count */
    unsigned timedout;   /* how many timed out */
    unsigned queue_full_count;  /* how many was dropped because of queue full */
    unsigned active_conn_count; /* a sum of active connections for the each new created. use for avg_conn_count = active_conn_count / conn_count */
    struct timeval total_time;  /* total time for all closed connections */
} ap_net_stat_t;

typedef int (*ap_net_conn_callback_func)(struct ap_net_connection_t *conn);

/* ********************************************************************** */
typedef struct ap_net_conn_pool_t
{
    struct ap_net_connection_t *conns; /* array */
    int max_connections; /* connections array size */
    int used_slots; /* count of used slots in conns array */
    int type; /* AP_NET_CTYPE_* */
    int status; /* AP_NET_ST_* */

    struct ap_net_poll_t *poller;

    struct timeval max_conn_ttl;   /*  max connection stall time. force closed after that */

    int listen_sock; /* <= 0 if no bind was made */
    struct sockaddr_in listen_addr;

    ap_net_conn_callback_func accept_func;  /* after creating new and init of struct ap_net_connection_t. init user data block here */
    ap_net_conn_callback_func close_func;   /* after closing connection in struct ap_net_connection_t */
    ap_net_conn_callback_func data_func;    /* after new data waiting detected at poll */
    ap_net_conn_callback_func destroy_func; /* before final destruction of struct ap_net_connection_t. chance to free user data */

    struct ap_net_stat_t stat;
} ap_net_conn_pool_t;

/* ********************************************************************** */
extern struct ap_net_conn_pool_t *ap_net_conn_pool_create(int type, int max_connections, int connection_timeout_ms,
                  ap_net_conn_callback_func accept_func, ap_net_conn_callback_func close_func, ap_net_conn_callback_func data_func,
                  ap_net_conn_callback_func destroy_func);

 /*  accepts new connection and adds it to the list. you must do ap_net_conn_pool_bind() first or initialize pool->listen_sock manually */
extern struct ap_net_connection_t *ap_net_conn_pool_accept_connection(struct ap_net_conn_pool_t *pool);
 /*  returns binded socket. -1 on any error. */
extern int  ap_net_conn_pool_bind(struct ap_net_conn_pool_t *pool, char *address, int port, int max_tries, int retry_sleep);
 /*  close tcp connection by index, msg !=NULL to post some answer before close (dangerous!) */
extern void ap_net_conn_pool_close_connection(struct ap_net_conn_pool_t *pool, int conn_idx, char *msg);

 /*  manual check for dropped connections */
extern int ap_net_conn_pool_check_conns(struct ap_net_conn_pool_t *pool);
extern int ap_net_conn_pool_connection_is_alive(struct ap_net_conn_pool_t *pool, int conn_idx); /*  returns true if alive */

/*  standalone poller functions: */
extern struct ap_net_poll_t *ap_net_poller_create(int listen_socket_fd, int max_connections);
extern int ap_net_poller_poll(struct ap_net_poll_t *poller);
extern int ap_net_poller_single_fd(int fd);

/*  conn_pool poller functions: */
extern int ap_net_conn_pool_poll(struct ap_net_conn_pool_t *pool);
extern int ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool);

extern int ap_net_conn_pool_recv(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int ap_net_conn_pool_send(struct ap_net_conn_pool_t *pool, int conn_idx, void *src_buf, int size);

extern void ap_net_conn_pool_print_stat(struct ap_net_conn_pool_t *pool); /*  print stats to debug channel */

extern int ap_net_conn_pool_set_max_connections(struct ap_net_conn_pool_t *pool, int new_max, int new_bufsize);

extern int ap_net_conn_pool_move_conn(struct ap_net_conn_pool_t *dst_pool, struct ap_net_conn_pool_t *src_pool, int conn_idx);

#endif
