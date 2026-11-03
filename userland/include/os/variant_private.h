/* os/variant_private.h stand-in (src/stubs/libxpc.c has the implementation: no internal build here) */
#ifndef __OS_VARIANT_PRIVATE_STANDIN_H__
#define __OS_VARIANT_PRIVATE_STANDIN_H__
#include <stdbool.h>
#include <sys/cdefs.h>

__BEGIN_DECLS
bool os_variant_has_internal_diagnostics(const char *subsystem);
bool os_variant_has_internal_content(const char *subsystem);
bool os_variant_has_internal_ui(const char *subsystem);
bool os_variant_allows_internal_security_policies(const char *subsystem);
__END_DECLS
#endif
