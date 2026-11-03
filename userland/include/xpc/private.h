/* xpc/private.h stand-in: the event publisher and entitlement calls libnotify makes (src/stubs/libxpc.c) */
#ifndef __XPC_PRIVATE_STANDIN_H__
#define __XPC_PRIVATE_STANDIN_H__
#include <xpc/xpc.h>

__BEGIN_DECLS
typedef struct _xpc_event_publisher_s *xpc_event_publisher_t;
int xpc_event_publisher_fire_noboost(xpc_event_publisher_t publisher, uint64_t token, xpc_object_t descriptor);
xpc_object_t xpc_copy_entitlement_for_token(const char *key, audit_token_t *token);
__END_DECLS
#endif
