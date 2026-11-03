/*
 * os_thread_self_restrict_rwx_*: per-thread RWX toggling through APRR/SPRR, which only Apple Silicon
 * has. On macOS they live in the closed part of libsystem_kernel; libpthread calls them for the JIT
 * write-protect API. The Cortex-A72 has no such feature, so report "not supported" and never switch.
 */
#include <stdbool.h>

bool
os_thread_self_restrict_rwx_is_supported(void)
{
	return false;
}

void
os_thread_self_restrict_rwx_to_rw(void)
{
}

void
os_thread_self_restrict_rwx_to_rx(void)
{
}
