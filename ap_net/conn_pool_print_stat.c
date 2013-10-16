/** \file ap_net/conn_pool_print_stat.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: Statistics output procedures
 */
#include "conn_pool_internals.h"

/* ********************************************************************** */
/** \brief Prints to debugging channel(s) summary from statistics gathered on given pool
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param intro_message char * - Introduction message printed before the numbers start showing. May be used to identify the pool if there is many of them
 * \return void
 */
void ap_net_conn_pool_print_stat(struct ap_net_conn_pool_t *pool, char *intro_message)
{
  long n;


  n = pool->stat.active_conn_count * 100 / pool->stat.conn_count;

  ap_log_debug_log("\n# ap_net statistics: %s\n\ttotal conns: %u, avg: %d.%02d, t/o count: %u, queue full: %u times\n",
            intro_message, pool->stat.conn_count, n / 100, n % 100, pool->stat.timedout, pool->stat.queue_full_count);

  n = pool->stat.total_time.tv_sec * 1000000 + pool->stat.total_time.tv_nsec / 1000000;

  ap_log_debug_log("\ttotal time: %ld sec, avg per conn: %ld.%03d\n", pool->stat.total_time.tv_sec, n / 1000000, n % 1000000);
}
