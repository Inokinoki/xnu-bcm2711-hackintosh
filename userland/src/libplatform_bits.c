/*
 * ffs/ffsl/fls/flsl: libplatform's generic files define them only for the simulator (VARIANT_DYLD with
 * TARGET_OS_SIMULATOR); on macOS the optimized versions are in closed assembly. Libc's qsort calls flsl.
 */
int
ffs(int mask)
{
	return mask ? __builtin_ctz((unsigned int)mask) + 1 : 0;
}

int
ffsl(long mask)
{
	return mask ? __builtin_ctzl((unsigned long)mask) + 1 : 0;
}

int
fls(int mask)
{
	return mask ? (int)(sizeof(mask) << 3) - __builtin_clz((unsigned int)mask) : 0;
}

int
flsl(long mask)
{
	return mask ? (int)(sizeof(mask) << 3) - __builtin_clzl((unsigned long)mask) : 0;
}
