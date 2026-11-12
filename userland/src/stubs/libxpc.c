/*
 * Stand-in for the parts of libxpc / liblaunch / libsystem_darwin that Libinfo and Libnotify call. The bootstrap_*
 * calls talk to the bootstrap server in /sbin/launchd (src/init/init.c) through the small protocol of
 * include/bootstrap_proto.h; a process that has no bootstrap port gets BOOTSTRAP_NOT_PRIVILEGED. xpc objects are
 * not implemented.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mach/mach.h>
#include "bootstrap_proto.h"

typedef void *xpc_object_t;

#define BS_TIMEOUT_MS 10000

/* one request/reply exchange with the bootstrap server; PORT is the right a BS_REGISTER carries */
static kern_return_t
bs_call(mach_port_t bp, int op, const char *name, mach_port_t port, mach_port_t *out)
{
	bs_request_t req;
	bs_reply_rcv_t rep;
	mach_port_t reply;
	kern_return_t kr;
	bool own_bp = false;

	if (out) {
		*out = MACH_PORT_NULL;
	}
	if (name == NULL || strlen(name) >= BS_NAME_MAX) {
		return KERN_INVALID_ARGUMENT;
	}
	if (bp == MACH_PORT_NULL) {
		if (task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bp) != KERN_SUCCESS || bp == MACH_PORT_NULL) {
			return BS_NOT_PRIVILEGED;
		}
		own_bp = true;
	}
	reply = mach_reply_port();

	memset(&req, 0, sizeof(req));
	req.head.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE) | MACH_MSGH_BITS_COMPLEX;
	req.head.msgh_size = sizeof(req);
	req.head.msgh_remote_port = bp;
	req.head.msgh_local_port = reply;
	req.head.msgh_id = op;
	req.body.msgh_descriptor_count = 1;
	req.port.name = port;
	req.port.disposition = MACH_MSG_TYPE_COPY_SEND;
	req.port.type = MACH_MSG_PORT_DESCRIPTOR;
	strlcpy(req.name, name, sizeof(req.name));

	kr = mach_msg(&req.head, MACH_SEND_MSG | MACH_RCV_MSG | MACH_SEND_TIMEOUT | MACH_RCV_TIMEOUT,
	    sizeof(req), sizeof(rep), reply, BS_TIMEOUT_MS, MACH_PORT_NULL);
	if (kr == KERN_SUCCESS) {
		if (rep.msg.head.msgh_id != op + BS_REPLY_ID || rep.msg.head.msgh_size != sizeof(rep.msg) ||
		    !(rep.msg.head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
			kr = KERN_FAILURE;
		} else if (rep.msg.ret != KERN_SUCCESS) {
			kr = rep.msg.ret;
		} else if (out) {
			*out = rep.msg.port.name;
		} else if (rep.msg.port.name != MACH_PORT_NULL) {
			mach_port_deallocate(mach_task_self(), rep.msg.port.name);
		}
	}
	mach_port_mod_refs(mach_task_self(), reply, MACH_PORT_RIGHT_RECEIVE, -1);
	if (own_bp) {
		mach_port_deallocate(mach_task_self(), bp);
	}
	return kr;
}

kern_return_t
bootstrap_look_up2(mach_port_t bp, const char *service_name, mach_port_t *sp, int pid, uint64_t flags)
{
	(void)pid; (void)flags;
	return bs_call(bp, BS_LOOK_UP, service_name, MACH_PORT_NULL, sp);
}

kern_return_t
bootstrap_look_up(mach_port_t bp, const char *service_name, mach_port_t *sp)
{
	return bs_call(bp, BS_LOOK_UP, service_name, MACH_PORT_NULL, sp);
}

kern_return_t
bootstrap_check_in(mach_port_t bp, const char *service_name, mach_port_t *sp)
{
	return bs_call(bp, BS_CHECK_IN, service_name, MACH_PORT_NULL, sp);
}

kern_return_t
bootstrap_register(mach_port_t bp, const char *service_name, mach_port_t sp)
{
	return bs_call(bp, BS_REGISTER, service_name, sp, NULL);
}

kern_return_t
bootstrap_register2(mach_port_t bp, const char *service_name, mach_port_t sp, uint64_t flags)
{
	(void)flags;
	return bs_call(bp, BS_REGISTER, service_name, sp, NULL);
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

/* libSystem_initializer calls these: libxpc has no state to set up or to reset in a forked child */
void _libxpc_initializer(void) { }
void xpc_atfork_prepare(void) { }
void xpc_atfork_parent(void) { }
void xpc_atfork_child(void) { }
