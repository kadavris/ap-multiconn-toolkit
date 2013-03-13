/*
dumb, anti-fool protection utilities
Made by AP for personal stuff
*/

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include "apstr.h"
#include "approtection.h"

int ap_protect_validate_key(char *key_file_content, t_ap_prot_key *decoded_key_out)
{
struct ifaddrs *ifaddr, *ifa;
int family, s;
char name[NI_MAXHOST];
struct sockaddr_ll *sll;

  if (getifaddrs(&ifaddr) == -1)
  {
    perror("getifaddrs");
    return 0;
  }

  /* Walk through linked list, maintaining head pointer so we can free list later */
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if ( family != AF_INET )
      continue;

    if (family == AF_INET || family == AF_INET6)
    {
      /* getting iterface's numerical address */
      s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
          name, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

      if (s != 0)
      {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        return 0;
      }

      if ( 0 == strcmp(name, ifaddr) )
      {
        /* saving interface addr */
        strcpy(name, ifa->ifa_name);
        break;
      }
    }
  }

  // looping in search of MAC info
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL
        || ifa->ifa_addr->sa_family != AF_PACKET
        || 0 != strcmp(name, ifa->ifa_name))
       continue;

    sll = (struct sockaddr_ll*)(ifa->ifa_addr);

    if ( sll->sll_hatype != ARPHRD_ETHER )
      continue;
    
    memdumpfd(fileno(stdout), sll, sizeof(struct sockaddr_ll));

    printf("sll_family: %u\nsll_protocol: %u\nsll_ifindex: %d\nsll_hatype: %u\nsll_pkttype: %u\nsll_halen: %u\nsll_addr:\n",
           (unsigned)(sll->sll_family), (unsigned)(sll->sll_protocol), sll->sll_ifindex, (unsigned)(sll->sll_hatype),
           (int)(sll->sll_pkttype), (int)(sll->sll_halen));

    memdumpfd(fileno(stdout), sll->sll_addr, 8);

    printf("\n");
  }

  freeifaddrs(ifaddr);
  return 1;
}
