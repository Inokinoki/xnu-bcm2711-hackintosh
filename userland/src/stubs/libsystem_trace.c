/*
 * libsystem_trace (os_log, os_trace) is closed. Libc reaches it through os_crash/os_assert: format the message
 * into the caller's buffer and send it nowhere. The os_log_* public entry points are not provided; callers that
 * look them up with dlsym (assumes.c) get NULL and take their fallback.
 */
#include <os/log_private.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct os_log_s {
	int unused;
};
struct os_log_s _os_log_default;

char *
os_log_send_and_compose(uint32_t flags, const char **fmt_out, char *buf, size_t buf_size, os_log_t log,
    os_log_type_t type, const char *fmt, ...)
{
	va_list ap;

	(void)flags;
	(void)log;
	(void)type;
	if (fmt_out) {
		*fmt_out = fmt;
	}
	if (buf && buf_size) {
		va_start(ap, fmt);
		vsnprintf(buf, buf_size, fmt, ap);
		va_end(ap);
	}
	return buf;
}

/* with OS_LOG_STDERR in the environment the (undecoded) format strings go to stderr: enough to see which call fired */
static void
emit_format(os_log_type_t type, const char *format)
{
	char buf[512];
	int n;

	if (!getenv("OS_LOG_STDERR")) {
		return;
	}
	n = snprintf(buf, sizeof(buf), "os_log[%d]: %s\n", (int)type, format);
	write(2, buf, (size_t)(n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1));
}

/* public os_log: everything is dropped (see above); os_log_type_enabled says no so callers skip building messages */
os_log_t
os_log_create(const char *subsystem, const char *category)
{
	(void)subsystem;
	(void)category;
	return &_os_log_default;
}

bool
os_log_type_enabled(os_log_t oslog, os_log_type_t type)
{
	(void)oslog;
	(void)type;
	return false;
}

void
_os_log_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)buf; (void)size;
	emit_format(type, format);
}

void
_os_log_debug_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)buf; (void)size;
	emit_format(type, format);
}

void
_os_log_error_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)buf; (void)size;
	emit_format(type, format);
}

void
_os_log_fault_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)buf; (void)size;
	emit_format(type, format);
}

/* libSystem_initializer: nothing to initialize, nothing to reset after fork */
void _libtrace_init(void) { }
void _libtrace_fork_child(void) { }

/* notifyd turns tracing off with this */
void os_trace_set_mode(uint32_t mode) { (void)mode; }
