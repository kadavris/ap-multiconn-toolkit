#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include "apstr.h"

       int
       main(int argc, char *argv[])
       {
           struct ifaddrs *ifaddr, *ifa;
           int family, s;
           char host[NI_MAXHOST];
           struct sockaddr_ll *sll;
           
           if (getifaddrs(&ifaddr) == -1) {
               perror("getifaddrs");
               exit(EXIT_FAILURE);
           }

           /* Walk through linked list, maintaining head pointer so we can free list later */

           for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
           {
               if (ifa->ifa_addr == NULL)
                   continue;

               family = ifa->ifa_addr->sa_family;

               /* Display interface name and family (including symbolic
                  form of the latter for the common families) */

               printf("%s  address family: %d%s\n",
                       ifa->ifa_name, family,
                       (family == AF_PACKET) ? " (AF_PACKET)" :
                       (family == AF_INET) ?   " (AF_INET)" :
                       (family == AF_INET6) ?  " (AF_INET6)" : "");

               /* For an AF_INET* interface address, display the address */

               if (family == AF_INET || family == AF_INET6) {
                   s = getnameinfo(ifa->ifa_addr,
                           (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                 sizeof(struct sockaddr_in6),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                   if (s != 0) {
                       printf("getnameinfo() failed: %s\n", gai_strerror(s));
                       exit(EXIT_FAILURE);
                   }
                   printf("\taddress: <%s>\n", host);
               }
               else if (family == AF_PACKET)
               {
                 sll = (struct sockaddr_ll*)(ifa->ifa_addr);
                 memdumpfd(fileno(stdout), sll, sizeof(struct sockaddr_ll));
                 printf("sll_family: %us\nsll_protocol: %us\nsll_ifindex: %d\nsll_hatype: %us\nsll_pkttype: %u\nsll_halen: %u\nsll_addr:\n",
                        sll->sll_family, sll->sll_protocol, sll->sll_ifindex, sll->sll_hatype,
                        (int)(sll->sll_pkttype), (int)(sll->sll_halen));
                 memdumpfd(fileno(stdout), sll->sll_addr, 8);
                 printf("\n");
               }
           }

           freeifaddrs(ifaddr);
           exit(EXIT_SUCCESS);
       }
