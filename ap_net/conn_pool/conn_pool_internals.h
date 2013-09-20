/* part of ap's toolkit.
 * ap_net/conn_pool internals definitions
 */

#include "../ap_net.h"
#include "../../ap_error/ap_error.h"
#include "../../ap_log.h"
#include "../../ap_str.h"
#include "../../ap_utils.h"

extern int ap_net_conn_pool_lock(struct ap_net_conn_pool_t *pool);

extern void ap_net_conn_pool_unlock(struct ap_net_conn_pool_t *pool);

extern struct ap_net_connection_t *ap_net_conn_pool_get_conn_by_fd(struct ap_net_conn_pool_t *pool, int fd);

extern int ap_net_tcprecv(int sh, void *buf, int size);

extern int ap_net_tcpsend(int sh, void *buf, int size);

extern int ap_net_conn_pool_poller_add_conn(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int ap_net_conn_pool_poller_remove_conn(struct ap_net_conn_pool_t *pool, int conn_idx);
extern int ap_net_conn_pool_poller_create(struct ap_net_conn_pool_t *pool);
