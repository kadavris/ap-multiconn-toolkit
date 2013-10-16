/** \file ap_net/conn_pool_internals.h
 * \brief Part of AP's Toolkit. Internal procedures definitions
 * \internal
 */
#define AP_NET_CONN_POOL

#include "ap_net.h"
#include "../ap_error/ap_error.h"
#include "../ap_log.h"
#include "../ap_str.h"
#include "../ap_utils.h"

extern int ap_net_recv(int sh, void *buf, int size, int non_blocking);
extern int ap_net_send(int sh, void *buf, int size, int non_blocking);

extern int ap_net_conn_pool_poller_add_conn(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int ap_net_conn_pool_poller_remove_conn(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool);

extern const char *ap_net_conn_pool_udp_conn_handshake;
