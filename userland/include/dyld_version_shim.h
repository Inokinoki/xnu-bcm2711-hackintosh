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
#ifndef dyld_platform_version_macOS_10_9
#define dyld_platform_version_macOS_10_9 ((dyld_build_version_t){PLATFORM_MACOS, 0x000a0900})
#endif
#ifndef dyld_platform_version_macOS_10_16
#define dyld_platform_version_macOS_10_16 ((dyld_build_version_t){PLATFORM_MACOS, 0x000a1000})
#endif
#ifndef dyld_platform_version_iOS_14_0
#define dyld_platform_version_iOS_14_0 ((dyld_build_version_t){PLATFORM_IOS, 0x000e0000})
#endif
#ifndef DYLD_MACOSX_VERSION_10_11
#define DYLD_MACOSX_VERSION_10_11 0x000A0B00
#endif
#endif
