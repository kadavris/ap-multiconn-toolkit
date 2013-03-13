#ifndef AP_TCP_H
#define AP_TCP_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

// tcp conn statuses (mostly internal for tcp_answer() func)
#define TC_ST_READY  0
#define TC_ST_BUSY   1
#define TC_ST_DATAIN 2
#define TC_ST_OUTPUT 3

typedef struct t_tcp_connection
{
  int fd; // file descriptor (0 = unused slot)
  int idx; // index in array
  struct sockaddr_in addr;
  struct timeval created_time, expire;
  char *buf; // IO buffer
  int nextline; // next line offset if last read() got too much. -1 if none, 0 if incomplete line
  int bufsize, bufptr; // buf size/current index
  // should this be an pointer to user-data...
  int state, cmdcode, needlines; // tcp_answer() internals
} t_tcp_connection;

typedef struct t_ap_tcpstat
{
  unsigned conn_count; // all time connections count
  unsigned timedout;   // how many timed out
  unsigned queue_full_count; // how many was dropped because of queue full
  unsigned active_conn_count; // a sum of active connections for the each new created. use for avg_conn_count = active_conn_count / conn_count
  struct timeval total_time; // total time for all closed connections
} t_ap_tcpstat;

#ifndef AP_TCP_C
extern int max_tcp_connections;   // max tcp connections to maintain
extern struct timeval max_tcp_conn_time;   // max tcp connection stall time. forced close after that)
extern struct t_tcp_connection *tcp_connections; // malloc'd on config read when 'max_tCPConnections' is known
extern int tcp_conn_count;
extern t_ap_tcpstat ap_tcpstat;
#endif

extern void tcp_connection_module_init(void);
extern int connrecv(int idx, void *buf, int size);
extern int connsend(int idx, void *buf, int size);
extern void tcp_close_connection(int idx, char *msg); // close tcp connection by index, msg !=NULL to post some answer before close
extern int tcp_accept_connection(int list_sock); // accepts new connection and adds it to the list
extern int tcp_check_state(int fd);
extern void tcp_check_conns(int a); // used as sigaction() EPIPE handler to prevent dumping when connection dropped unexpectedly
extern void tcp_print_stat(void); // print stats to debug channel
#endif
