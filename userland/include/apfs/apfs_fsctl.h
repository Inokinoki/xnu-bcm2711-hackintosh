/*
 * apfs/apfs_fsctl.h is Apple-internal. Libc uses two APFS fsctls: the firmlink check in getcwd and directory
 * statistics in libdarwin. This system has no APFS; on any other file system both fsctls fail, which the
 * callers handle (getcwd assumes a firmlink and falls back to lstat, dirstat falls back to walking). The
 * request numbers and layouts only need to compile.
 */
#ifndef __APFS_FSCTL_H__
#define __APFS_FSCTL_H__

#include <stdint.h>
#include <sys/ioccom.h>

#define FIRMLINK_GET 1

typedef struct apfs_firmlink_control {
	uint32_t cmd;
	uint32_t val;
} apfs_firmlink_control_t;

#define APFSIOC_FIRMLINK_CTL _IOWR('J', 90, apfs_firmlink_control_t)

#define APFS_DIR_STATS_FAST_PATH 0x1

struct apfs_dir_stats_ext {
	uint64_t flags;
	uint64_t total_size;
	uint64_t num_children;
	uint64_t reserved[5];
};

#define APFSIOC_GET_DIR_STATS_EXT _IOWR('J', 88, struct apfs_dir_stats_ext)

/* removefile's purgeable-space flag: an fsctl only APFS answers */
#define APFS_CLEAR_PURGEABLE 0x1
#define APFSIOC_MARK_PURGEABLE _IOW('J', 80, uint64_t)

#endif /* __APFS_FSCTL_H__ */
