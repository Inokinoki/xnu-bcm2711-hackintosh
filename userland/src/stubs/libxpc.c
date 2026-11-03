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

bool os_variant_has_internal_diagnostics(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_has_internal_content(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_has_internal_ui(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_allows_internal_security_policies(const char *subsystem) { (void)subsystem; return false; }
