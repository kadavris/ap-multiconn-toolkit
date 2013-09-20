#include "ap_tcp.h"
#include "../ap_utils.h"

/*
// tcp conn statuses (mostly internal for tcp_answer() func)
#define TC_ST_READY  0
#define TC_ST_BUSY   1
#define TC_ST_DATAIN 2
#define TC_ST_OUTPUT 3

typedef struct ap_tcp_connection_t
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
} ap_tcp_connection_t;

typedef struct ap_tcp_stat_t
{
  unsigned conn_count; // all time connections count
  unsigned timedout;   // how many timed out
  unsigned queue_full_count; // how many was dropped because of queue full
  unsigned active_conn_count; // a sum of active connections for the each new created. use for avg_conn_count = active_conn_count / conn_count
  struct timeval total_time; // total time for all closed connections
} ap_tcp_stat_t;

extern struct ap_tcp_connection_t *ap_tcp_connections; // malloc'd on config read when 'max_tCPConnections' is known
extern int ap_tcp_conn_count;
extern struct ap_tcp_stat_t ap_tcp_stat;

extern int  ap_tcp_accept_connection(int list_sock); // accepts new connection and adds it to the list
extern void ap_tcp_check_conns(int dummy); // used as sigaction() EPIPE handler to prevent dumping when connection dropped unexpectedly
extern int  ap_tcp_check_state(int fd);
extern void ap_tcp_close_connection(int conn_idx, char *msg); // close tcp connection by index, msg !=NULL to post some answer before close
extern int  ap_tcp_conn_recv(int conn_idx, void *buf, int size);
extern int  ap_tcp_conn_send(int conn_idx, void *buf, int size);
extern void ap_tcp_print_stat(void); // print stats to debug channel
*/

/*
  making client/server here
*/

void main(void)
{
  printf("test: create and destroy 1000 pools with full init\n");
  fflush(stdout);

  ap_tcp_connection_module_init(1000, 3000);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);

  printf("test: \n");
  fflush(stdout);
}

