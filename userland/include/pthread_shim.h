/*
 * Declarations libpthread-514 expects from headers that are not in the open-source releases. Pulled in
 * with -include when building libpthread (LIB_EXTRA_CFLAGS in userland/build-xcode-lib.sh).
 */
#include <stdbool.h>
extern bool _os_xbs_chrooted; /* defined in libsyscall's _libkernel_init.c */
