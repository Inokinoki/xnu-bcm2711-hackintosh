/*
 * os/thread_self_restrict.h is Apple-internal: per-thread write protection of dyld's allocator through TPRO
 * (a feature of newer Apple Silicon; the Cortex-A72 has none). With TPRO off dyld never calls these, but the
 * calls must still compile. Pulled in with -include (LIB_EXTRA_CFLAGS in the workflow), as dyld does not include
 * it itself.
 */
#ifndef __DYLD_TPRO_STANDIN_H__
#define __DYLD_TPRO_STANDIN_H__
#ifndef __ASSEMBLER__ /* -include also reaches the .s files */

#include <stdbool.h>

static inline bool os_thread_self_restrict_tpro_is_writable(void) { return true; }
static inline void os_thread_self_restrict_tpro_to_rw(void) { }
static inline void os_thread_self_restrict_tpro_to_ro(void) { }

#endif
#endif /* __DYLD_TPRO_STANDIN_H__ */
