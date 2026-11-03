/*
 * libsystem_darwin (libdarwin) in Libc builds only with Apple's internal headers (os/boot_mode_private.h, ...).
 * What the other libraries import from it is the os_variant family: this is not an internal build, so every
 * answer is "no". Libraries are bound to libsystem_darwin by ordinal, so these must live here, not in libxpc.
 */
#include <stdbool.h>

bool os_variant_has_internal_diagnostics(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_has_internal_content(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_has_internal_ui(const char *subsystem) { (void)subsystem; return false; }
bool os_variant_allows_internal_security_policies(const char *subsystem) { (void)subsystem; return false; }

void __libdarwin_init(void) { }
