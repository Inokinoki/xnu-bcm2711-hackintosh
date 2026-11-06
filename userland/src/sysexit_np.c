/*
 * sysexit_np comes from libdarwin, which is closed. mount only uses it to turn an errno into a sysexits(3) exit
 * status (errno_or_sysexit in mount.c), so this is an approximation of that mapping.
 */
#include <errno.h>
#include <sysexits.h>

int sysexit_np(int err)
{
	switch (err) {
	case 0:
		return EX_OK;
	case EPERM: case EACCES: case EROFS:
		return EX_NOPERM;
	case ENOENT: case ENOTDIR: case ENXIO: case ENODEV:
		return EX_NOINPUT;
	case EINVAL: case E2BIG: case ENAMETOOLONG:
		return EX_USAGE;
	case EEXIST: case ENOSPC: case EDQUOT:
		return EX_CANTCREAT;
	case ENOMEM:
		return EX_OSERR;
	case EBUSY: case EAGAIN: case ETIMEDOUT:
		return EX_TEMPFAIL;
	case ENOTSUP:
		return EX_UNAVAILABLE;
	default:
		return EX_IOERR;
	}
}
