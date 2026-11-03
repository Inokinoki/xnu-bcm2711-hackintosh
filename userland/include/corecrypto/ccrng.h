/*
 * corecrypto is closed. Libc's arc4random needs only three entry points: a generator handle, bytes and a
 * uniform number below a bound. Implemented here on getentropy(2), which the kernel serves from its PRNG,
 * so libsystem_c gets no dependency on libcorecrypto.
 */
#ifndef __CORECRYPTO_CCRNG_H__
#define __CORECRYPTO_CCRNG_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include <unistd.h>

struct ccrng_state {
	int unused;
};

static inline struct ccrng_state *
ccrng(int *error)
{
	static struct ccrng_state state;
	if (error) {
		*error = 0;
	}
	return &state;
}

static inline int
ccrng_generate(struct ccrng_state *rng, size_t n, void *buf)
{
	(void)rng;
	uint8_t *p = buf;
	while (n) {
		size_t chunk = n > 256 ? 256 : n; /* getentropy(2) limit */
		if (getentropy(p, chunk) != 0) {
			return -1;
		}
		p += chunk;
		n -= chunk;
	}
	return 0;
}

/* uniform in [0, bound): rejection sampling */
static inline int
ccrng_uniform(struct ccrng_state *rng, uint64_t bound, uint64_t *rand_out)
{
	uint64_t r, limit;
	if (bound == 0) {
		return -1;
	}
	limit = UINT64_MAX - (UINT64_MAX % bound) - 1;
	do {
		if (ccrng_generate(rng, sizeof(r), &r) != 0) {
			return -1;
		}
	} while (r > limit && limit != UINT64_MAX);
	*rand_out = r % bound;
	return 0;
}

#endif /* __CORECRYPTO_CCRNG_H__ */
