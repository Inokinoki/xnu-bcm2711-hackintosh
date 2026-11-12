/*
 * The wire protocol between the bootstrap server in /sbin/launchd (src/init/init.c) and the bootstrap_* calls of
 * the libxpc stand-in (src/stubs/libxpc.c). Apple's own protocol runs through launchd's XPC domain, which is
 * closed; both ends are ours, so this is a single MIG-shaped request/reply pair instead.
 *
 * Every request and reply is a complex message with exactly one port descriptor (MACH_PORT_NULL when the call
 * has no port to carry), so neither side has to cope with two layouts.
 *   BS_CHECK_IN  name              -> RECEIVE right of a new port that the server keeps a send right to
 *   BS_REGISTER  name + SEND right -> (none)
 *   BS_LOOK_UP   name              -> SEND right
 */
#ifndef BOOTSTRAP_PROTO_H
#define BOOTSTRAP_PROTO_H
#include <stdint.h>
#include <mach/mach.h>

#define BS_NAME_MAX 128

enum {
	BS_CHECK_IN = 400,
	BS_REGISTER = 401,
	BS_LOOK_UP  = 402,
	BS_REPLY_ID = 100,    /* the reply to request N has id N + BS_REPLY_ID */
};

/* values of servers/bootstrap.h */
#define BS_NOT_PRIVILEGED   1100
#define BS_NAME_IN_USE      1101
#define BS_UNKNOWN_SERVICE  1102
#define BS_SERVICE_ACTIVE   1103

typedef struct {
	mach_msg_header_t          head;
	mach_msg_body_t            body;
	mach_msg_port_descriptor_t port;
	char                       name[BS_NAME_MAX];
} bs_request_t;

typedef struct {
	mach_msg_header_t          head;
	mach_msg_body_t            body;
	mach_msg_port_descriptor_t port;
	kern_return_t              ret;
} bs_reply_t;

/* receive buffers: the kernel appends a trailer */
typedef struct {
	bs_request_t          msg;
	mach_msg_max_trailer_t trailer;
} bs_request_rcv_t;

typedef struct {
	bs_reply_t            msg;
	mach_msg_max_trailer_t trailer;
} bs_reply_rcv_t;

#endif
