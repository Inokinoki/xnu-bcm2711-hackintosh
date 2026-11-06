/* APFS.framework is private and closed. diskdev_cmds' mount.c calls APFSContainerMigrateMediaKeys on APFS
 * containers only; there are none on this system. */
#ifndef APFS_STANDIN_H
#define APFS_STANDIN_H
#include <mach/kern_return.h>
static inline kern_return_t APFSContainerMigrateMediaKeys(const char *container) { (void)container; return KERN_NOT_SUPPORTED; }
#endif
