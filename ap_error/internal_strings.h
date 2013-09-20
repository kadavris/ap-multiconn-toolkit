/* this is internal only stuff for error reporting procedures */
#ifndef AP_ERROR_INTERNAL_H
#define AP_ERROR_INTERNAL_H

const char *ap_log_errno_strings[] =
{
    "No error", /* 0 */
    "System error", /* 1 */
    "Out of memory", /* 2 */
    "Connections list is full. dumping new incoming", /* 3 */
    "epoll_create()", /* 4 */
    "epoll_wait()", /* 5 */
};

#endif
