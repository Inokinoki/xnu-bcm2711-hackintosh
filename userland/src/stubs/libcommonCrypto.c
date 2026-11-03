/*
 * libcommonCrypto (CommonCrypto/CommonDigest.h) is closed, built on corecrypto. Only SHA-256 so far, which `sort`
 * uses for its random ordering; written from FIPS 180-4.
 */
#include <stdint.h>
#include <string.h>

typedef struct CC_SHA256state_st {
	uint32_t count[2];
	uint32_t hash[8];
	uint32_t wbuf[16];
} CC_SHA256_CTX;

static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void
block(CC_SHA256_CTX *c, const uint8_t *p)
{
	uint32_t w[64], a, b, cc, d, e, f, g, h, t1, t2;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
	}
	for (i = 16; i < 64; i++) {
		uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);

		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	a = c->hash[0]; b = c->hash[1]; cc = c->hash[2]; d = c->hash[3];
	e = c->hash[4]; f = c->hash[5]; g = c->hash[6]; h = c->hash[7];
	for (i = 0; i < 64; i++) {
		t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
		t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & cc) ^ (b & cc));
		h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
	}
	c->hash[0] += a; c->hash[1] += b; c->hash[2] += cc; c->hash[3] += d;
	c->hash[4] += e; c->hash[5] += f; c->hash[6] += g; c->hash[7] += h;
}

int
CC_SHA256_Init(CC_SHA256_CTX *c)
{
	static const uint32_t iv[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

	memcpy(c->hash, iv, sizeof(iv));
	c->count[0] = c->count[1] = 0; /* bytes, low and high */
	return 1;
}

int
CC_SHA256_Update(CC_SHA256_CTX *c, const void *data, uint32_t len)
{
	const uint8_t *p = data;
	uint32_t fill = c->count[0] & 63;

	if ((c->count[0] += len) < len) {
		c->count[1]++;
	}
	while (len) {
		uint32_t n = 64 - fill;

		if (n > len) {
			n = len;
		}
		memcpy((uint8_t *)c->wbuf + fill, p, n);
		fill += n; p += n; len -= n;
		if (fill == 64) {
			block(c, (uint8_t *)c->wbuf);
			fill = 0;
		}
	}
	return 1;
}

int
CC_SHA256_Final(unsigned char *md, CC_SHA256_CTX *c)
{
	uint32_t fill = c->count[0] & 63;
	uint64_t bits = ((uint64_t)c->count[1] << 32 | c->count[0]) * 8;
	uint8_t pad[72] = { 0x80 };
	uint32_t padlen = (fill < 56 ? 56 : 120) - fill;
	int i;

	for (i = 0; i < 8; i++) {
		pad[padlen + i] = (uint8_t)(bits >> (56 - 8 * i));
	}
	CC_SHA256_Update(c, pad, padlen + 8);
	for (i = 0; i < 8; i++) {
		md[4 * i] = c->hash[i] >> 24; md[4 * i + 1] = c->hash[i] >> 16;
		md[4 * i + 2] = c->hash[i] >> 8; md[4 * i + 3] = c->hash[i];
	}
	return 1;
}
