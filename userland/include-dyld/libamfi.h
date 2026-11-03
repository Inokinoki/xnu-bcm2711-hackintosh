/*
 * libamfi.h is the closed interface to AppleMobileFileIntegrity. dyld asks it which environment variables,
 * @-paths and interposing a process may use, given whether the main executable is restricted. This system has
 * no AMFI: allow everything for an ordinary process, and for a restricted one (restrict segment, setuid) keep
 * dyld's own environment variables and custom shared caches off.
 */
#ifndef __LIBAMFI_H__
#define __LIBAMFI_H__

#include <stdint.h>

enum amfi_dyld_policy_input_flag_set {
	AMFI_DYLD_INPUT_PROC_IN_SIMULATOR = (1 << 0),
	AMFI_DYLD_INPUT_PROC_HAS_RESTRICT_SEG = (1 << 1),
	AMFI_DYLD_INPUT_PROC_IS_ENCRYPTED = (1 << 2),
};

enum amfi_dyld_policy_output_flag_set {
	AMFI_DYLD_OUTPUT_ALLOW_AT_PATH = (1 << 0),
	AMFI_DYLD_OUTPUT_ALLOW_PATH_VARS = (1 << 1),
	AMFI_DYLD_OUTPUT_ALLOW_CUSTOM_SHARED_CACHE = (1 << 2),
	AMFI_DYLD_OUTPUT_ALLOW_FALLBACK_PATHS = (1 << 3),
	AMFI_DYLD_OUTPUT_ALLOW_PRINT_VARS = (1 << 4),
	AMFI_DYLD_OUTPUT_ALLOW_FAILED_LIBRARY_INSERTION = (1 << 5),
	AMFI_DYLD_OUTPUT_ALLOW_LIBRARY_INTERPOSING = (1 << 6),
	AMFI_DYLD_OUTPUT_ALLOW_EMBEDDED_VARS = (1 << 7),
};

static inline int
amfi_check_dyld_policy_self(uint64_t input_flags, uint64_t *output_flags)
{
	uint64_t out = AMFI_DYLD_OUTPUT_ALLOW_AT_PATH | AMFI_DYLD_OUTPUT_ALLOW_FALLBACK_PATHS |
	    AMFI_DYLD_OUTPUT_ALLOW_FAILED_LIBRARY_INSERTION | AMFI_DYLD_OUTPUT_ALLOW_LIBRARY_INTERPOSING |
	    AMFI_DYLD_OUTPUT_ALLOW_EMBEDDED_VARS;

	if (!(input_flags & AMFI_DYLD_INPUT_PROC_HAS_RESTRICT_SEG)) {
		out |= AMFI_DYLD_OUTPUT_ALLOW_PATH_VARS | AMFI_DYLD_OUTPUT_ALLOW_CUSTOM_SHARED_CACHE |
		    AMFI_DYLD_OUTPUT_ALLOW_PRINT_VARS;
	}
	*output_flags = out;
	return 0;
}

#endif /* __LIBAMFI_H__ */
