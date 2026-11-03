/*
 * libsystem_trace (os_log, os_trace) is closed. Libc reaches it through os_crash/os_assert: format the message
 * into the caller's buffer and send it nowhere. The os_log_* public entry points are not provided; callers that
 * look them up with dlsym (assumes.c) get NULL and take their fallback.
 */
#include <os/log_private.h>
#include <stdarg.h>
#include <stdio.h>

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
