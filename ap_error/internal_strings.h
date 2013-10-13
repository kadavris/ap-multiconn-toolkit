/* Part of AP's Toolkit
 *  this is internal only stuff for error reporting procedures
 **/
#ifndef AP_ERROR_INTERNAL_H
#define AP_ERROR_INTERNAL_H

const char *ap_log_errno_strings[] =
{
    "No error", /* 0 */
    "System error", /* 1 */
    "Special case", /* 2 */
    "Out of memory", /* 3 */
    "Connections list is full. dumping new incoming", /* 4 */
    "Invalid index of connection in pool", /* 5 */
    "Locked for too long", /* 6 */
    "Pool's IP version is different than address in connect()", /* 7 */
    "User's callback denied connection accept", /* 8 */
    NULL
};

#endif
