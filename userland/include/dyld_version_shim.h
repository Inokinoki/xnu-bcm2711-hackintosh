/*
 * dyld_platform_version_macOS_* are generated into the SDK's dyld_priv.h, not present in dyld's sources.
 * Pulled in with -include for libmalloc (LIB_EXTRA_CFLAGS in userland/build-xcode-lib.sh).
 */
#ifndef __ASSEMBLER__ /* -include also reaches the .s files */
#include <mach-o/loader.h>
#include <mach-o/dyld_priv.h>
#ifndef dyld_platform_version_macOS_13_0
#define dyld_platform_version_macOS_13_0 ((dyld_build_version_t){PLATFORM_MACOS, 0x000d0000})
#endif
#endif
