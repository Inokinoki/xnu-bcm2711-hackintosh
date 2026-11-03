/*
 * os/alloc_once_private.h: not in the open-source releases (libpthread, Libc, libplatform's asl.c and keymgr
 * include it; libplatform ships only alloc_once_impl.h, which refuses direct inclusion). This is the
 * wrapper those projects expect. The keys index _os_alloc_once_table, which is private to our own
 * libraries, so the numbering is ours; it only has to be unique and below OS_ALLOC_ONCE_KEY_MAX.
 */
#ifndef __OS_ALLOC_ONCE__
#define __OS_ALLOC_ONCE__

#define __OS_ALLOC_INDIRECT__
#include <os/alloc_once_impl.h>
#undef __OS_ALLOC_INDIRECT__

#define OS_ALLOC_ONCE_KEY_LIBSYSTEM_C		0
#define OS_ALLOC_ONCE_KEY_LIBSYSTEM_BLOCKS	1
#define OS_ALLOC_ONCE_KEY_LIBSYSTEM_MALLOC	2
#define OS_ALLOC_ONCE_KEY_LIBDISPATCH		3
#define OS_ALLOC_ONCE_KEY_LIBXPC		4
#define OS_ALLOC_ONCE_KEY_LIBINFO		5
#define OS_ALLOC_ONCE_KEY_LIBCACHE		6
#define OS_ALLOC_ONCE_KEY_LIBKEYMGR		7
#define OS_ALLOC_ONCE_KEY_OS_TRACE		8
#define OS_ALLOC_ONCE_KEY_LIBSYSTEM_KERNEL	9
#define OS_ALLOC_ONCE_KEY_LIBPTHREAD		10
#define OS_ALLOC_ONCE_KEY_LIBNOTIFY		11
#define OS_ALLOC_ONCE_KEY_LIBPLATFORM		12

#endif /* __OS_ALLOC_ONCE__ */
