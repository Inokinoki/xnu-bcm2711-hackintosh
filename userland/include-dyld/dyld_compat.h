/*
 * Force-included into every dyld source (-include in the workflow): what dyld gets from Apple's internal SDK
 * headers transitively but not from the macOS 13.5 headers we build against. Nothing for assembly.
 */
#ifndef __DYLD_COMPAT_H__
#define __DYLD_COMPAT_H__
#ifndef __ASSEMBLER__

#include <fcntl.h>
#include <os/thread_self_restrict.h>

#endif
#endif /* __DYLD_COMPAT_H__ */
