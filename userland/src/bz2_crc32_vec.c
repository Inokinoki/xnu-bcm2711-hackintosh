/*
 * bzip2-45 calls Apple's crc32_vec (arm64/crc32vec.s) for the block CRC. That assembly uses PMULL, which the
 * Cortex-A72 of the Raspberry Pi 4 does not have (no crypto extension), so this is the plain table version:
 * the same unfinalised CRC update over len bytes that the bzlib.c call site expects.
 */
#include "bzlib_private.h"

int crc32_vec(int crc, char *p, int len)
{
	UInt32 c = (UInt32)crc;
	while (len-- > 0) {
		BZ_UPDATE_CRC(c, *p++);
	}
	return (int)c;
}
