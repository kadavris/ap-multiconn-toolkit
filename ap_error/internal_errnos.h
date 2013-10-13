/* Part of AP's Toolkit
 *  this is internal only stuff for error reporting procedures
 **/
#ifndef INTERNAL_ERRNOS_H
#define INTERNAL_ERRNOS_H

#define AP_ERRNO_NOERROR              0
#define AP_ERRNO_SYSTEM               1
#define AP_ERRNO_CUSTOM_MESSAGE       2
#define AP_ERRNO_OOM                  3
#define AP_ERRNO_CONNLIST_FULL        4
#define AP_ERRNO_INVALID_CONN_INDEX   5
#define AP_ERRNO_LOCKED               6
#define AP_ERRNO_BAD_PROTO            7
#define AP_ERRNO_ACCEPT_DENIED        8
/* don't forget to update internal_strings.h with error description !! */

#endif
