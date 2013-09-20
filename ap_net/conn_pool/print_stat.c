#define AP_NET_CONN_POOL

#include "conn_pool_internals.h"

/* ********************************************************************** */
void ap_net_conn_pool_print_stat(struct ap_net_conn_pool_t *pool)
{
  long n;


  n = pool->stat.active_conn_count * 100 / pool->stat.conn_count;

  ap_log_debug_log("\n# ap_net: total conns: %u, avg: %d.%02d, t/o count: %u, queue full: %u times\n",
            pool->stat.conn_count, n / 100, n % 100, pool->stat.timedout, pool->stat.queue_full_count);

  n = pool->stat.total_time.tv_sec * 1000000 + pool->stat.total_time.tv_usec / 1000;

  ap_log_debug_log("# ap_net: total time: %ld sec, avg per conn: %ld.%03d\n", pool->stat.total_time.tv_sec, n / 1000000, n % 1000000);
}
