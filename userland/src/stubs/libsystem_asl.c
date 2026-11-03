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
