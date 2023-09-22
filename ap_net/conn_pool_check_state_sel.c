/** \file ap_net/conn_pool_check_state_sel.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: connection state checking via select() procedure
 */
#include "conn_pool_internals.h"

/*
static const char *_func_name = "ap_net_conn_pool_check_state_sel()";
*/

/* ********************************************************************** */
/** \brief Checking connection state, old way via select()
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \return int bitmask: b1 = data waiting, b2 = can send, b3 = error
 *
 * \deprecated
 */
int ap_net_conn_pool_check_state_sel(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    struct timeval tv;
    fd_set fdr, fdw, fde;
    int n, fd;


    ap_error_clear();

    if ( conn_idx < 0 || conn_idx >= pool->max_connections )
      return -1;

    fd = pool->conns[conn_idx].fd;

    if( fd <= 0 )
        return -1;

    n = send(fd, &n, 0, 0);

    if ( n == -1 && errno == EPIPE )
    {
        if (ap_log_debug_level > 10)
            ap_log_debug_log("- ap_net_check_state(%d): send(%d): %d/%m\n", conn_idx, fd, n);

        return 4;
    }

    /*
    According to the mans, what's down below is total bullshit.
    You always have to monitor for EPIPE and EAGAIN errors
    and, possibly for 0 bytes read return as eof() indication specifically.
    The read and write states on non-blocked socket are always ON.
    */
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    FD_ZERO(&fde);

    FD_SET(fd, &fdr);
    FD_SET(fd, &fdw);
    FD_SET(fd, &fde);

    n = fd + 1;
    tv.tv_sec = tv.tv_usec = 0;

    n = select(n, &fdr, &fdw, &fde, &tv);

    if ( ap_log_debug_level > 10 )
        ap_log_debug_log("- ap_net_check_state(%d): select() error: %m\n", conn_idx);

    if ( n <= 0 ) return n;

    n = 0;
    if ( FD_ISSET(fd, &fdr) )
        n = 1;

    if ( FD_ISSET(fd, &fdw) )
        n |= 2;

    if ( FD_ISSET(fd, &fde) )
        n |= 4;

    if ( ap_log_debug_level > 10 )
        ap_log_debug_log("- ap_net_check_state(%d): return %d\n", conn_idx, n);

    return n;
}
