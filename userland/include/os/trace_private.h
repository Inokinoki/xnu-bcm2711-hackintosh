/* os/trace_private.h stand-in: the one call notifyd makes (src/stubs/libsystem_trace.c) */
#ifndef __OS_TRACE_PRIVATE_STANDIN_H__
#define __OS_TRACE_PRIVATE_STANDIN_H__
#include <stdint.h>
#include <sys/cdefs.h>

#define OS_TRACE_MODE_DISABLE 0x80000000u

__BEGIN_DECLS
void os_trace_set_mode(uint32_t mode);
__END_DECLS
#endif
