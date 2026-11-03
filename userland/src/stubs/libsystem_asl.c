/*
 * libsystem_asl is closed. Libc's syslog is implemented there (the $DARWIN_EXTSN variants are what the SDK headers
 * bind to): messages go to the console of the calling process's stderr when it is a terminal-less daemon is not
 * worth the trouble, so they are dropped unless OS_LOG_STDERR is set. openlog/setlogmask keep their state-free contract.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* With OS_LOG_STDERR set in the environment, syslog lines go to stderr so a failing first boot can be read */
static void
emit(int priority, const char *message, va_list args)
{
	char buf[1024];
	int n;

	if (!getenv("OS_LOG_STDERR")) {
		return;
	}
	n = snprintf(buf, sizeof(buf), "<%d> ", priority);
	n += vsnprintf(buf + n, sizeof(buf) - (size_t)n - 1, message, args);
	if (n > (int)sizeof(buf) - 2) {
		n = (int)sizeof(buf) - 2;
	}
	buf[n++] = '\n';
	write(2, buf, (size_t)n);
}

void syslog_ext(int priority, const char *message, ...) __asm("_syslog$DARWIN_EXTSN");
void vsyslog_ext(int priority, const char *message, va_list args) __asm("_vsyslog$DARWIN_EXTSN");
void vsyslog(int priority, const char *message, va_list args) { emit(priority, message, args); }
void vsyslog_ext(int priority, const char *message, va_list args) { emit(priority, message, args); }
void syslog(int priority, const char *message, ...) { va_list ap; va_start(ap, message); emit(priority, message, ap); va_end(ap); }
void syslog_ext(int priority, const char *message, ...) { va_list ap; va_start(ap, message); emit(priority, message, ap); va_end(ap); }
void openlog(const char *ident, int logopt, int facility) { (void)ident; (void)logopt; (void)facility; }
void closelog(void) { }

int
setlogmask(int mask)
{
	static int current = 0xff;
	int old = current;

	if (mask != 0) {
		current = mask;
	}
	return old;
}

/* libsystem_asl's _simple_asl_log, used by Libnotify and Libc's _simple.c */
void _simple_asl_log(int level, const char *facility, const char *message) { (void)level; (void)facility; (void)message; }

/* the asl client API (libsystem_c and Libinfo call it for the occasional log line): every call fails or returns NULL */
void *asl_new(unsigned int type) { (void)type; return 0; }
void *asl_open(const char *ident, const char *facility, unsigned int opts) { (void)ident; (void)facility; (void)opts; return 0; }
int asl_set(void *obj, const char *key, const char *value) { (void)obj; (void)key; (void)value; return -1; }
int asl_set_query(void *obj, const char *key, const char *value, unsigned int op) { (void)obj; (void)key; (void)value; (void)op; return -1; }
const char *asl_get(void *obj, const char *key) { (void)obj; (void)key; return 0; }
int asl_append(void *obj, void *other) { (void)obj; (void)other; return -1; }
int asl_send(void *obj, void *msg) { (void)obj; (void)msg; return -1; }
int asl_log(void *client, void *msg, int level, const char *format, ...) { (void)client; (void)msg; (void)level; (void)format; return -1; }
void *asl_match(void *client, void *query, unsigned long long *last, unsigned long long start, unsigned int count, unsigned int duration, int dir) { (void)client; (void)query; (void)last; (void)start; (void)count; (void)duration; (void)dir; return 0; }
void *asl_next(void *list) { (void)list; return 0; }
void asl_release(void *obj) { (void)obj; }
void asl_free(void *obj) { (void)obj; }
