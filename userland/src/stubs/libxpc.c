/*
 * Stand-in for the parts of libxpc / liblaunch / libsystem_darwin that Libinfo and Libnotify call. There is no
 * launchd bootstrap server yet: every look-up fails with BOOTSTRAP_UNKNOWN_SERVICE, which both libraries handle
 * (notify falls back to local-only operation). xpc objects are not implemented.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <mach/mach.h>

#define BOOTSTRAP_UNKNOWN_SERVICE 1102

typedef void *xpc_object_t;

kern_return_t
bootstrap_look_up2(mach_port_t bp, const char *service_name, mach_port_t *sp, int pid, uint64_t flags)
{
	(void)bp; (void)service_name; (void)pid; (void)flags;
	*sp = MACH_PORT_NULL;
	return BOOTSTRAP_UNKNOWN_SERVICE;
}

int
xpc_event_publisher_fire_noboost(void *publisher, uint64_t token, xpc_object_t descriptor)
{
	(void)publisher; (void)token; (void)descriptor;
	return 0;
}

xpc_object_t xpc_copy_entitlement_for_token(const char *key, void *token) { (void)key; (void)token; return NULL; }
xpc_object_t xpc_dictionary_create(const char *const *keys, const xpc_object_t *values, size_t count) { (void)keys; (void)values; (void)count; return NULL; }
void xpc_dictionary_set_string(xpc_object_t xdict, const char *key, const char *string) { (void)xdict; (void)key; (void)string; }
void xpc_dictionary_set_uint64(xpc_object_t xdict, const char *key, uint64_t value) { (void)xdict; (void)key; (void)value; }
xpc_object_t xpc_create_from_plist(const void *data, size_t size) { (void)data; (void)size; return NULL; }
void xpc_release(xpc_object_t object) { (void)object; }

struct _xpc_bool_s { int unused; } _xpc_bool_true, _xpc_bool_false;


/* more of the xpc surface that copyfile and libutil link against */
struct _xpc_type_s { int unused; } _xpc_type_dictionary;

bool _xpc_runtime_is_app_sandboxed(void) { return false; }
const void *xpc_get_type(xpc_object_t object) { (void)object; return NULL; }
xpc_object_t xpc_connection_create_mach_service(const char *name, void *targetq, uint64_t flags) { (void)name; (void)targetq; (void)flags; return NULL; }
void xpc_connection_set_event_handler(xpc_object_t connection, void *handler) { (void)connection; (void)handler; }
void xpc_connection_resume(xpc_object_t connection) { (void)connection; }
xpc_object_t xpc_connection_send_message_with_reply_sync(xpc_object_t connection, xpc_object_t message) { (void)connection; (void)message; return NULL; }
uint64_t xpc_dictionary_get_uint64(xpc_object_t xdict, const char *key) { (void)xdict; (void)key; return 0; }

/* liblaunch: no parent bootstrap port, and no launchd job state to swap */
kern_return_t bootstrap_parent(mach_port_t bp, mach_port_t *parent_port) { (void)bp; *parent_port = MACH_PORT_NULL; return 1100; /* BOOTSTRAP_NOT_PRIVILEGED */ }
void *vproc_swap_integer(void *vp, int key, int64_t *inval, int64_t *outval) { (void)vp; (void)key; (void)inval; if (outval) *outval = 0; return 0; }
