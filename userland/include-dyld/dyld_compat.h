/*
 * Force-included into every dyld source (-include in the workflow): what dyld gets from Apple's internal SDK
 * headers transitively but not from the macOS 13.5 headers we build against. Nothing for assembly.
 */
#ifndef __DYLD_COMPAT_H__
#define __DYLD_COMPAT_H__
#ifndef __ASSEMBLER__

#include <fcntl.h>
#include "os/thread_self_restrict.h" /* ours, next to this file: <os/...> finds the xnu header, which has no TPRO calls */

/* old name of PLATFORM_MACCATALYST, gone from the SDK headers but still used by DyldAPIs.cpp */
#ifndef PLATFORM_IOSMAC
#define PLATFORM_IOSMAC 6
#endif

#endif
#endif /* __DYLD_COMPAT_H__ */
