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

/* public os_log: everything is dropped; os_log_type_enabled says no so callers skip building messages */
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
	(void)dso; (void)log; (void)type; (void)format; (void)buf; (void)size;
}

void
_os_log_debug_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)type; (void)format; (void)buf; (void)size;
}

void
_os_log_error_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)type; (void)format; (void)buf; (void)size;
}

void
_os_log_fault_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size)
{
	(void)dso; (void)log; (void)type; (void)format; (void)buf; (void)size;
}
