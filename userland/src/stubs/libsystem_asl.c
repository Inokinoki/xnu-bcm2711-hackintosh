/*
 * libsystem_asl is closed. Libc's syslog is implemented there (the $DARWIN_EXTSN variants are what the SDK headers
 * bind to): messages go to the console of the calling process's stderr when it is a terminal-less daemon is not
 * worth the trouble, so they are dropped. openlog/setlogmask keep their state-free contract.
 */
#include <stdarg.h>

void syslog_ext(int priority, const char *message, ...) __asm("_syslog$DARWIN_EXTSN");
void vsyslog_ext(int priority, const char *message, va_list args) __asm("_vsyslog$DARWIN_EXTSN");
void syslog(int priority, const char *message, ...) { (void)priority; (void)message; }
void vsyslog(int priority, const char *message, va_list args) { (void)priority; (void)message; (void)args; }
void syslog_ext(int priority, const char *message, ...) { (void)priority; (void)message; }
void vsyslog_ext(int priority, const char *message, va_list args) { (void)priority; (void)message; (void)args; }
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
