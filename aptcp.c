/*
  aptcp.c: sockets manipulation and alike functions. written by Andrej Pakhutin for his own use primarily.
*/
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>

#define AP_TCP_C
#include "aptcp.h"
#include "apstr.h"

int max_tcp_connections = 3;   // default max tcp connections to maintain
struct timeval max_tcp_conn_time;   // max tcp connection stall time. forced close after that)
struct t_tcp_connection *tcp_connections = NULL; // malloc'd on config read when 'max_tcp_connections' is known
int tcp_conn_count = 0; // current connections count
t_ap_tcpstat ap_tcpstat; // statistics companion

//=================================================================
void tcp_connection_module_init(void)
{
  int i;

  tcp_connections = getmem(max_tcp_connections * sizeof(struct t_tcp_connection), "malloc on tcp_connections");

  for (i = 0; i < max_tcp_connections; ++i)
  {
    tcp_connections[i].fd = 0;
    tcp_connections[i].bufptr = 0;
    tcp_connections[i].bufsize = 1024;
    tcp_connections[i].buf = getmem(tcp_connections[i].bufsize, "malloc on tcp_connections.buf");
  }

  ap_tcpstat.conn_count = 0;
  ap_tcpstat.active_conn_count = 0;
  ap_tcpstat.timedout = 0;
  ap_tcpstat.queue_full_count = 0;
  ap_tcpstat.total_time.tv_sec = 0;
  ap_tcpstat.total_time.tv_usec = 0;
}

//=================================================================
int tcprecv(int sh, void *buf, int size)
{
  if ( -1 != fcntl(sh, F_GETFL) )
    return recv(sh, buf, size, MSG_DONTWAIT);

  if (debug_level > 9)
    debuglog("! sock %d recv error: %m\n", sh);

  return -1;
}

//=================================================================
int tcpsend(int sh, void *buf, int size)
{
  if ( -1 != fcntl(sh, F_GETFL) )
    return send(sh, buf, size, MSG_DONTWAIT);

  if (debug_level > 9)
    debuglog("! sock %d send error: %m\n", sh);

  return -1;
}

//=================================================================
int connrecv(int idx, void *buf, int size)
{
  int n;

  errno = 0;

  if ( 0 == tcp_connections[idx].fd )
  {
    dosyslog(LOG_ERR, "! connrecv() on closed fd");
    return -1;
  }

  n = recv(tcp_connections[idx].fd, buf, size, 0);

  if ( (n == -1 && n == EPIPE) || n == 0)
  {
    if (debug_level)
      debuglog("? TCP Connection [%d] is dead prematurely\n", idx);

    tcp_close_connection(idx, NULL);
  }

  return n;
}

//=================================================================
int connsend(int idx, void *buf, int size)
{
  int n;

  if ( 0 == tcp_connections[idx].fd )
  {
    dosyslog(LOG_ERR, "! connrecv() on closed fd");
    return 0;
  }

  n = send(tcp_connections[idx].fd, buf, size, 0);

  if ( (n == -1 && n == EPIPE) || n == 0)
  {
    if (debug_level)
      debuglog("? TCP Connection #%d is dead prematurely: %m\n", idx);

    tcp_close_connection(idx, NULL);
  }
  return n;
}

//=======================================================================
void tcp_close_connection(int idx, char *msg) // close tcp connection by index, msg !=NULL to post some answer before close
{
  struct timeval tv;
  int dh;

  if ( 0 == tcp_connections[idx].fd )
    return;

  dh = is_debug_handle(tcp_connections[idx].fd);
  remove_debug_handle(tcp_connections[idx].fd);

  if ( msg != NULL )
    connsend(idx, msg, strlen(msg));

  close(tcp_connections[idx].fd);

  tcp_connections[idx].fd = 0;
  --tcp_conn_count;

  if ( ! dh ) // debug connections will not count for execution time
  {
    gettimeofday(&tv, NULL);
    timersub(&tv, &tcp_connections[idx].created_time, &tv);
    timeradd(&ap_tcpstat.total_time, &tv, &ap_tcpstat.total_time);
  }

  if ( debug_level > 0 )
    debuglog("* TCP conn [%d] closed\n", idx);
}

//=======================================================================
int tcp_accept_connection(int list_sock) // accepts new connection and adds it to the list
{
  int tcpci, n, new_sock;

  for (tcpci = 0; tcpci < max_tcp_connections; ++tcpci) // finding free slot
    if (tcp_connections[tcpci].fd == 0)
      break;

  if (tcpci == max_tcp_connections)
  {
     ++ap_tcpstat.queue_full_count;

     if (debug_level)
       debuglog("? Conn list is full. dumping new incoming\n");

     return 0;
  }

  n = sizeof(tcp_connections[tcpci].addr);

  if ( -1 == (new_sock = accept(list_sock, (struct sockaddr *)&tcp_connections[tcpci].addr, (socklen_t*)&n)) )
  {
    dosyslog(LOG_ERR, "! accept: %m");
    return 0;
  }

  /* setting non-blocking connection.
     data exchange will be performed in the timer_event() called by alarm handler
  */
  fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL) | O_NONBLOCK);

  tcp_connections[tcpci].fd = new_sock;
  tcp_connections[tcpci].idx = tcpci;

  gettimeofday(&tcp_connections[tcpci].created_time, NULL);
  timeradd(&tcp_connections[tcpci].created_time, &max_tcp_conn_time, &tcp_connections[tcpci].expire);

  tcp_connections[tcpci].bufptr = 0;
  tcp_connections[tcpci].state = TC_ST_READY;
  ++tcp_conn_count;

  ++ap_tcpstat.conn_count;
  ap_tcpstat.active_conn_count += tcp_conn_count;

  if (debug_level) debuglog("* Got connected ([%d])\n", tcpci);

  return 1;
}

//=======================================================================
int tcp_check_state(int fd)
{
  struct timeval tv;
  fd_set fdr, fdw, fde;
  int n;

  n = send(fd, &n, 0, 0);

  if ( n == -1 && n == EPIPE )
  {
    if (debug_level > 10)
      debuglog("- tcp_check_state(%d): send(): %d/%m\n", fd, n);

    return 4;
  }

  /*
  according to the mans what's down below is total bullshit.
  you always have to monitor for EPIPE and EAGAIN errors
  and, possibly for 0 bytes read return as eof() indication specifically.
  The read and write states on non-blocked socket are always ON.
  */
  FD_ZERO(&fdr); FD_ZERO(&fdw); FD_ZERO(&fde);
  FD_SET(fd, &fdr); FD_SET(fd, &fdw); FD_SET(fd, &fde);

  n = fd + 1;
  tv.tv_sec = tv.tv_usec = 0;

  n = select(n, &fdr, &fdw, &fde, &tv);

  if ( n <= 0 ) return n;

  if ( FD_ISSET(fd, &fdr) ) n = 1;
  if ( FD_ISSET(fd, &fdw) ) n |= 2;
  if ( FD_ISSET(fd, &fde) ) n |= 4;
  //if (debug_level > 10 && debug_to_tTY) fprintf(stderr, "- tcp_check_state(%d): %d\n", fd, n);

  return n;
}

//=======================================================================
// used as sigaction() EPIPE handler to prevent dumping when connection dropped unexpectedly
void tcp_check_conns(int a)
{
  int i,n;

  for (i = 0; i < max_tcp_connections; ++i)
  {
    if ( tcp_connections[i].fd == 0 )
      continue;

    n = tcp_check_state(tcp_connections[i].fd);

    if ( n != -1 && ((n & 4) == 0) )
      continue;

    tcp_close_connection(i, NULL);

    if (debug_level)
      debuglog("\n? [%d] Session dropped/error\n", i);
  }
}

//=======================================================================
void tcp_print_stat(void)
{
  long n;

  n = ap_tcpstat.active_conn_count * 100u / ap_tcpstat.conn_count;

  debuglog("\n# aptcp: total conns: %u, avg: %u.%02u, t/o count: %u, queue full: %u times\n",
    ap_tcpstat.conn_count, n/100u, n%100u, ap_tcpstat.timedout, ap_tcpstat.queue_full_count);

  n = ap_tcpstat.total_time.tv_sec * 1000000000u + ap_tcpstat.total_time.tv_usec;

  debuglog("# aptcp: total time: %ld sec, avg per conn: %ld.%03ld\n", ap_tcpstat.total_time.tv_sec,
    n / 1000000000u, n % 1000000000u);
}

/*/=======================================================================
int tcp_get_MACs(void)
{
struct ifreq ifr;
struct ifconf ifc;
char buf[1024];
int success = 0;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1)
  {
  // handle error
  }

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
  {
  // handle error
  }

  struct ifreq* it = ifc.ifc_req;
  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

  for (; it != end; ++it)
  {
    strcpy(ifr.ifr_name, it->ifr_name);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
    {
      if (! (ifr.ifr_flags & IFF_LOOPBACK))
      { // don't count loopback
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
        {
          success = 1;
          break;
        }
      }
    }
    else
    {
    // handle error
    }
  }

  unsigned char mac_address[6];

  if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
}
*/