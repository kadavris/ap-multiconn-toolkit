/** \file ap_net/conn_pool_set_addr.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: IP address assigning procedures
 */
#include "conn_pool_internals.h"

static const char *err_bad_buf_len = "bad address buffer length";
static const char *err_bad_port = "bad port: %d";

/* ********************************************************************** */
/** \brief Fills struct sockaddr_in* with provided data
 *
 * \param af int Address family (AF_INET or AF_INET6)
 * \param destination void* pointer to struct sockaddr_in* where converted address will be stored
 * \param address_str char* textual representation of IP address or host name
 * \param addr_len length of destination buffer
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
*/
int ap_net_set_str_addr(int af, void *destination, const char *address_str, socklen_t addr_len, int port)
{
	const char *_func_name = "ap_net_set_str_addr()";


    if ( af == AF_INET6 )
    {
    	if ( addr_len < sizeof(struct sockaddr_in6))
    	{
    		ap_error_set_custom(_func_name, (char*)err_bad_buf_len);
            return 0;
    	}

    	((struct sockaddr_in6 *)destination)->sin6_family = AF_INET6;
    	((struct sockaddr_in6 *)destination)->sin6_port = htons(port);
    	destination = &(((struct sockaddr_in6 *)destination)->sin6_addr);
    }
    else if ( af == AF_INET )
    {
    	if ( addr_len < sizeof(struct sockaddr_in))
    	{
    		ap_error_set_custom(_func_name, (char*)err_bad_buf_len);
            return 0;
    	}

    	((struct sockaddr_in *)destination)->sin_family = AF_INET;
    	((struct sockaddr_in *)destination)->sin_port = htons(port);
    	destination = &(((struct sockaddr_in *)destination)->sin_addr);
    }
    else
    {
        ap_error_set_custom(_func_name, "bad address family: %d", af);
        return 0;
    }

    if ( -1 == inet_pton(af, address_str, destination) )
    {
         ap_error_set_custom(_func_name, "bad address: %s", address_str);
         return 0;
    }

    return 1;
}

/*************************************************************/
/** \brief Fills struct sockaddr_in with provided data
 *
 * \param destination struct sockaddr_in * pointer to struct sockaddr_in where converted address will be stored
 * \param address4 - Numeric representation of IPv4 address such as INADDR_LOOPBACK
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
 *
 * Address and port are converted to network byte order on assignment
*/
int ap_net_set_ip4_addr(struct sockaddr_in *destination, in_addr_t address4, int port)
{
	if ( port <= 0 || port > 65535 )
    {
         ap_error_set_custom("ap_net_conn_pool_set_ip4_addr()", (char*)err_bad_port, port);
         return 0;
    }

	destination->sin_family = AF_INET;
	destination->sin_port = htons(port);
	destination->sin_addr.s_addr = htonl(address4);

    return 1;
}

/*************************************************************/
/** \brief Fills struct sockaddr_in6 with provided data
 *
 * \param destination struct sockaddr_in6 * pointer to struct sockaddr_in6 where converted address will be stored
 * \param address6 - Numeric representation of IPv6 address such as IN6ADDR_LOOPBACK_INIT
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
 *
 * Port is converted to network byte order on assignment
*/
int ap_net_set_ip6_addr(struct sockaddr_in6 *destination, struct in6_addr *address6, int port)
{
	if ( port <= 0 || port > 65535 )
    {
         ap_error_set_custom("ap_net_conn_pool_set_ip6_addr()", (char*)err_bad_port, port);
         return 0;
    }

    /*TODO: STUB. should check if it is right method: */
	destination->sin6_family = AF_INET6;
    destination->sin6_port = htons(port);
    memcpy(&destination->sin6_addr, address6, sizeof(struct in6_addr));

    return 1;
}

/* ********************************************************************** */
/** \brief Set pool's listener socket address
 *
 * \param pool struct ap_net_conn_pol_t* pointer to pool
 * \param address_str char* string representation of binding address
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
*/
int ap_net_conn_pool_set_str_addr(struct ap_net_conn_pool_t *pool, const char *address_str, int port)
{
    int ip6;


    ip6 = bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6);

    return ap_net_set_str_addr( (ip6 ? AF_INET6 : AF_INET), (ip6 ? (void*)&pool->listener.addr6 : (void*)&pool->listener.addr4),
                                 address_str, (ip6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)), port);
    /**TODO: check for correct address type */
}

/*************************************************************/
/** \brief Sets pool's listener address with IPv4 data
 *
 * \param pool struct ap_net_conn_pool_t* pointer to struct sockaddr_in6 where converted address will be stored
 * \param address4 in_addr_t - Numeric representation of IPv4 address such as INADDR_LOOPBACK
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
 *
 * Address and port are converted to network byte order on assignment
*/
int ap_net_conn_pool_set_ip4_addr(struct ap_net_conn_pool_t *pool, in_addr_t address4, int port)
{
    if ( bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) )
    {
        ap_error_set_custom("ap_net_conn_pool_set_ip4_addr()", "Pool is set to IPv6 mode");
        return 0;
    }

    return ap_net_set_ip4_addr(&pool->listener.addr4, address4, port);
}

/*************************************************************/
/** \brief Sets pool's listener socket address to IPv6 data
 *
 * \param pool struct ap_net_conn_pool_t* pointer to struct sockaddr_in6 where converted address will be stored
 * \param address6 - Numeric representation of IPv6 address such as IN6ADDR_LOOPBACK_INIT
 * \param port int - listener port
 * \return 0 on error. 1 if OK.
 *
 * Port is converted to network byte order on assignment
*/
int ap_net_conn_pool_set_ip6_addr(struct ap_net_conn_pool_t *pool, struct in6_addr *address6, int port)
{
    if ( ! bit_is_set(pool->flags, AP_NET_POOL_FLAGS_IPV6) )
    {
        ap_error_set_custom("ap_net_conn_pool_set_ip6_addr()", "Pool is set to IPv4 mode");
        return 0;
    }

    return ap_net_set_ip6_addr(&pool->listener.addr6, address6, port);
}

