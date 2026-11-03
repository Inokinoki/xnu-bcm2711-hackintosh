/*
 * sandbox/private.h belongs to the closed Sandbox framework. dyld asks sandbox_check whether a path or syscall is
 * denied before probing it; there is no sandbox here, so nothing is ever denied (<= 0 means "allowed" to dyld).
 */
#ifndef __SANDBOX_PRIVATE_H__
#define __SANDBOX_PRIVATE_H__

#include <sys/types.h>

enum sandbox_filter_type {
	SANDBOX_FILTER_NONE = 0,
	SANDBOX_FILTER_PATH = 1,
	SANDBOX_FILTER_SYSCALL_NUMBER = 18,
	SANDBOX_CHECK_NO_REPORT = 0x40000000,
};
typedef enum sandbox_filter_type sandbox_filter_type;

static inline int
sandbox_check(pid_t pid, const char *operation, sandbox_filter_type type, ...)
{
	(void)pid;
	(void)operation;
	(void)type;
	return 0;
}

#endif /* __SANDBOX_PRIVATE_H__ */
