/*
 * corecrypto is closed. dyld uses its digest interface to hash code-signature directories (SHA-1, SHA-256,
 * SHA-384); this is a portable implementation of just that interface, header-only so that it links into
 * dyld's static, libc-free image. Not constant-time and not meant for anything but hashing public data.
 */
#ifndef __CORECRYPTO_CCDIGEST_H__
#define __CORECRYPTO_CCDIGEST_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct ccdigest_info {
	size_t output_size;
	size_t state_size;
	void (*init)(void *ctx);
	void (*update)(void *ctx, size_t len, const void *data);
	void (*final)(void *ctx, void *out);
};

#define ccdigest_di_decl(di, name) uint64_t name[((di)->state_size + 7) / 8]
#define ccdigest_di_clear(di, name) memset(name, 0, sizeof(name))
#define ccdigest_init(di, ctx) ((di)->init(ctx))
#define ccdigest_update(di, ctx, len, data) ((di)->update(ctx, len, data))
#define ccdigest_final(di, ctx, out) ((di)->final(ctx, out))

static inline void
ccdigest(const struct ccdigest_info *di, size_t len, const void *data, void *out)
{
	uint64_t ctx[(di->state_size + 7) / 8];
	di->init(ctx);
	di->update(ctx, len, data);
	di->final(ctx, out);
	memset(ctx, 0, sizeof(ctx));
}

/* ---- SHA-1 and SHA-256: 64-byte blocks ---- */

struct cc_sha32_ctx {
	uint32_t h[8];
	uint64_t total;
	uint8_t buf[64];
	uint32_t n;
};

#define CC_ROR32(x, k) (((x) >> (k)) | ((x) << (32 - (k))))
#define CC_ROL32(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

static inline void
cc_sha1_block(uint32_t h[8], const uint8_t *p)
{
	uint32_t w[80], a, b, c, d, e, f, k, t;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
	}
	for (i = 16; i < 80; i++) {
		w[i] = CC_ROL32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	}
	a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
	for (i = 0; i < 80; i++) {
		if (i < 20) {
			f = (b & c) | (~b & d); k = 0x5a827999;
		} else if (i < 40) {
			f = b ^ c ^ d; k = 0x6ed9eba1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc;
		} else {
			f = b ^ c ^ d; k = 0xca62c1d6;
		}
		t = CC_ROL32(a, 5) + f + e + k + w[i];
		e = d; d = c; c = CC_ROL32(b, 30); b = a; a = t;
	}
	h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

static inline void
cc_sha256_block(uint32_t h[8], const uint8_t *p)
{
	static const uint32_t K[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
	uint32_t w[64], v[8], t1, t2;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
	}
	for (i = 16; i < 64; i++) {
		uint32_t s0 = CC_ROR32(w[i - 15], 7) ^ CC_ROR32(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = CC_ROR32(w[i - 2], 17) ^ CC_ROR32(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	for (i = 0; i < 8; i++) {
		v[i] = h[i];
	}
	for (i = 0; i < 64; i++) {
		t1 = v[7] + (CC_ROR32(v[4], 6) ^ CC_ROR32(v[4], 11) ^ CC_ROR32(v[4], 25)) + ((v[4] & v[5]) ^ (~v[4] & v[6])) + K[i] + w[i];
		t2 = (CC_ROR32(v[0], 2) ^ CC_ROR32(v[0], 13) ^ CC_ROR32(v[0], 22)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
		v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
		v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
	}
	for (i = 0; i < 8; i++) {
		h[i] += v[i];
	}
}

static inline void
cc_sha32_update(struct cc_sha32_ctx *c, void (*block)(uint32_t *, const uint8_t *), size_t len, const void *data)
{
	const uint8_t *p = (const uint8_t *)data;

	c->total += len;
	while (len) {
		size_t take = 64 - c->n;
		if (take > len) {
			take = len;
		}
		memcpy(c->buf + c->n, p, take);
		c->n += (uint32_t)take;
		p += take;
		len -= take;
		if (c->n == 64) {
			block(c->h, c->buf);
			c->n = 0;
		}
	}
}

static inline void
cc_sha32_final(struct cc_sha32_ctx *c, void (*block)(uint32_t *, const uint8_t *), size_t outwords, void *out)
{
	uint64_t bits = c->total * 8;
	uint8_t pad = 0x80, zero = 0, lenbytes[8];
	uint8_t *o = (uint8_t *)out;
	size_t i;

	for (i = 0; i < 8; i++) {
		lenbytes[i] = (uint8_t)(bits >> (56 - 8 * i));
	}
	cc_sha32_update(c, block, 1, &pad);
	while (c->n != 56) {
		cc_sha32_update(c, block, 1, &zero);
	}
	cc_sha32_update(c, block, 8, lenbytes);
	for (i = 0; i < outwords; i++) {
		o[4 * i] = (uint8_t)(c->h[i] >> 24);
		o[4 * i + 1] = (uint8_t)(c->h[i] >> 16);
		o[4 * i + 2] = (uint8_t)(c->h[i] >> 8);
		o[4 * i + 3] = (uint8_t)c->h[i];
	}
}

static inline void
cc_sha1_init(void *ctx)
{
	struct cc_sha32_ctx *c = (struct cc_sha32_ctx *)ctx;
	static const uint32_t iv[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
	memset(c, 0, sizeof(*c));
	memcpy(c->h, iv, sizeof(iv));
}

static inline void
cc_sha1_update(void *ctx, size_t len, const void *data)
{
	cc_sha32_update((struct cc_sha32_ctx *)ctx, cc_sha1_block, len, data);
}

static inline void
cc_sha1_final(void *ctx, void *out)
{
	cc_sha32_final((struct cc_sha32_ctx *)ctx, cc_sha1_block, 5, out);
}

static inline void
cc_sha256_init(void *ctx)
{
	struct cc_sha32_ctx *c = (struct cc_sha32_ctx *)ctx;
	static const uint32_t iv[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	memset(c, 0, sizeof(*c));
	memcpy(c->h, iv, sizeof(iv));
}

static inline void
cc_sha256_update(void *ctx, size_t len, const void *data)
{
	cc_sha32_update((struct cc_sha32_ctx *)ctx, cc_sha256_block, len, data);
}

static inline void
cc_sha256_final(void *ctx, void *out)
{
	cc_sha32_final((struct cc_sha32_ctx *)ctx, cc_sha256_block, 8, out);
}

/* ---- SHA-384: SHA-512 core, 128-byte blocks ---- */

struct cc_sha64_ctx {
	uint64_t h[8];
	uint64_t total;
	uint8_t buf[128];
	uint32_t n;
};

#define CC_ROR64(x, k) (((x) >> (k)) | ((x) << (64 - (k))))

static inline void
cc_sha512_block(uint64_t h[8], const uint8_t *p)
{
	static const uint64_t K[80] = {
		0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
		0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
		0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
		0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
		0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
		0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
		0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
		0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
		0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
		0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
		0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
		0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
		0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
		0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
		0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
		0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
		0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
		0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
		0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
		0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
	};
	uint64_t w[80], v[8], t1, t2;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = 0;
		for (int j = 0; j < 8; j++) {
			w[i] = (w[i] << 8) | p[8 * i + j];
		}
	}
	for (i = 16; i < 80; i++) {
		uint64_t s0 = CC_ROR64(w[i - 15], 1) ^ CC_ROR64(w[i - 15], 8) ^ (w[i - 15] >> 7);
		uint64_t s1 = CC_ROR64(w[i - 2], 19) ^ CC_ROR64(w[i - 2], 61) ^ (w[i - 2] >> 6);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	for (i = 0; i < 8; i++) {
		v[i] = h[i];
	}
	for (i = 0; i < 80; i++) {
		t1 = v[7] + (CC_ROR64(v[4], 14) ^ CC_ROR64(v[4], 18) ^ CC_ROR64(v[4], 41)) + ((v[4] & v[5]) ^ (~v[4] & v[6])) + K[i] + w[i];
		t2 = (CC_ROR64(v[0], 28) ^ CC_ROR64(v[0], 34) ^ CC_ROR64(v[0], 39)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
		v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
		v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
	}
	for (i = 0; i < 8; i++) {
		h[i] += v[i];
	}
}

static inline void
cc_sha384_init(void *ctx)
{
	struct cc_sha64_ctx *c = (struct cc_sha64_ctx *)ctx;
	static const uint64_t iv[8] = {
		0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL, 0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
		0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL, 0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
	};
	memset(c, 0, sizeof(*c));
	memcpy(c->h, iv, sizeof(iv));
}

static inline void
cc_sha384_update(void *ctx, size_t len, const void *data)
{
	struct cc_sha64_ctx *c = (struct cc_sha64_ctx *)ctx;
	const uint8_t *p = (const uint8_t *)data;

	c->total += len;
	while (len) {
		size_t take = 128 - c->n;
		if (take > len) {
			take = len;
		}
		memcpy(c->buf + c->n, p, take);
		c->n += (uint32_t)take;
		p += take;
		len -= take;
		if (c->n == 128) {
			cc_sha512_block(c->h, c->buf);
			c->n = 0;
		}
	}
}

static inline void
cc_sha384_final(void *ctx, void *out)
{
	struct cc_sha64_ctx *c = (struct cc_sha64_ctx *)ctx;
	uint64_t bits = c->total * 8;
	uint8_t pad = 0x80, zero = 0, lenbytes[16];
	uint8_t *o = (uint8_t *)out;
	size_t i;

	memset(lenbytes, 0, 8); /* high 64 bits of the 128-bit length */
	for (i = 0; i < 8; i++) {
		lenbytes[8 + i] = (uint8_t)(bits >> (56 - 8 * i));
	}
	cc_sha384_update(c, 1, &pad);
	while (c->n != 112) {
		cc_sha384_update(c, 1, &zero);
	}
	cc_sha384_update(c, 16, lenbytes);
	for (i = 0; i < 6; i++) {
		for (int j = 0; j < 8; j++) {
			o[8 * i + j] = (uint8_t)(c->h[i] >> (56 - 8 * j));
		}
	}
}

static const struct ccdigest_info cc_sha1_info = { 20, sizeof(struct cc_sha32_ctx), cc_sha1_init, cc_sha1_update, cc_sha1_final };
static const struct ccdigest_info cc_sha256_info = { 32, sizeof(struct cc_sha32_ctx), cc_sha256_init, cc_sha256_update, cc_sha256_final };
static const struct ccdigest_info cc_sha384_info = { 48, sizeof(struct cc_sha64_ctx), cc_sha384_init, cc_sha384_update, cc_sha384_final };

static inline const struct ccdigest_info *ccsha1_di(void) { return &cc_sha1_info; }
static inline const struct ccdigest_info *ccsha256_di(void) { return &cc_sha256_info; }
static inline const struct ccdigest_info *ccsha384_di(void) { return &cc_sha384_info; }

#endif /* __CORECRYPTO_CCDIGEST_H__ */
