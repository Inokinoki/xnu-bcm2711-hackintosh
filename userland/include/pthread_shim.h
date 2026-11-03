/*
 * Declarations libpthread-514 expects from headers that are not in the open-source releases. Pulled in
 * with -include when building libpthread (LIB_EXTRA_CFLAGS in userland/build-xcode-lib.sh).
 */
#ifndef __ASSEMBLER__ /* -include also reaches the .s files */
#include <stdbool.h>
extern bool _os_xbs_chrooted; /* defined in libsyscall's _libkernel_init.c */
#endif
