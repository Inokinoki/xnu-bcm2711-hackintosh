/* xpc/private.h stand-in: the event publisher and entitlement calls libnotify makes (src/stubs/libxpc.c) */
#ifndef __XPC_PRIVATE_STANDIN_H__
#define __XPC_PRIVATE_STANDIN_H__
#include <xpc/xpc.h>

__BEGIN_DECLS
typedef struct _xpc_event_publisher_s *xpc_event_publisher_t;
typedef enum {
	XPC_EVENT_PUBLISHER_ACTION_ADD = 1,
	XPC_EVENT_PUBLISHER_ACTION_REMOVE = 2,
	XPC_EVENT_PUBLISHER_ACTION_INITIAL_BARRIER = 3,
} xpc_event_publisher_action_t;
typedef void (^xpc_event_publisher_handler_t)(xpc_event_publisher_action_t action, uint64_t token, xpc_object_t descriptor);
typedef void (^xpc_event_publisher_error_handler_t)(int error);
xpc_event_publisher_t xpc_event_publisher_create(const char *name, void *queue);
void xpc_event_publisher_set_handler(xpc_event_publisher_t publisher, xpc_event_publisher_handler_t handler);
void xpc_event_publisher_set_error_handler(xpc_event_publisher_t publisher, xpc_event_publisher_error_handler_t handler);
void xpc_event_publisher_activate(xpc_event_publisher_t publisher);
uint64_t xpc_event_publisher_get_subscriber_asid(xpc_event_publisher_t publisher, uint64_t token);
int xpc_event_publisher_fire_noboost(xpc_event_publisher_t publisher, uint64_t token, xpc_object_t descriptor);
xpc_object_t xpc_copy_entitlement_for_token(const char *key, audit_token_t *token);
__END_DECLS
#endif
