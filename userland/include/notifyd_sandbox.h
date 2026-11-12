/* force-included when building notifyd: the sandbox constants pathwatch.c and notify_proc.c use, which only Apple's internal
 * <sandbox/private.h> defines. The values do not matter: sandbox_check_by_audit_token always allows (libsystem_darwin stand-in). */
#ifndef NOTIFYD_SANDBOX_STANDIN_H
#define NOTIFYD_SANDBOX_STANDIN_H
#define SANDBOX_CHECK_NO_REPORT 0x40000000
#define SANDBOX_FILTER_NONE 0
#define SANDBOX_FILTER_PATH 1
#define SANDBOX_FILTER_NOTIFICATION 6
#include <errno.h>    /* notify_proc.c uses errno without including it */
#endif
