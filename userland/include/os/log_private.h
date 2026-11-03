/*
 * os/log_private.h belongs to libtrace (os_log), which is closed. Libc's assumes.c and libdarwin include it
 * for the os_log_pack type; the log functions themselves are looked up with dlsym at run time and simply
 * absent here. Only the public os/log.h types plus the pack layout are provided.
 */
#ifndef __OS_LOG_PRIVATE_H__
#define __OS_LOG_PRIVATE_H__

#include <os/log.h>
#include <stdint.h>
#include <time.h>

typedef struct os_log_pack_s {
	uint64_t olp_continuous_time;
	struct timespec olp_wall_time;
	const void *olp_mh;
	const void *olp_pc;
	const char *olp_format;
	uint8_t olp_data[0];
} os_log_pack_s, *os_log_pack_t;

#define OS_LOG_F_SEND 0x01
#define OS_LOG_F_COMPOSE 0x02

/* libtrace; our libsystem_trace stand-in provides it (formats into buf, sends nothing) */
extern char *os_log_send_and_compose(uint32_t flags, const char **fmt_out, char *buf, size_t buf_size,
		os_log_t log, os_log_type_t type, const char *fmt, ...);

#endif /* __OS_LOG_PRIVATE_H__ */
