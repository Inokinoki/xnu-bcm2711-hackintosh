/*
 * libcommonCrypto (CommonCrypto/*.h) is closed, built on corecrypto. This stand-in provides the digests (MD5, SHA-1,
 * SHA-224/256/384/512), HMAC, PBKDF2, AES (CCCrypt, CBC and ECB, optional PKCS#7 padding) and the random bytes call
 * on top of BearSSL's portable code (MD5, SHA-1, SHA-384/512, AES constant-time 32-bit), the same files the kernel
 * stages in xnu/patches-8796.141.3/0025-stage-bearssl.sh. SHA-256 (and SHA-224, which only differs in the initial
 * value and the truncation) is the hand-written code below, written from FIPS 180-4: the CC_SHA256_CTX of the SDK is
 * a ready-made state for it, and `sort` has been using it since the first boot.
 *
 * BearSSL contexts begin with a vtable pointer, which the CC_*_CTX structs of the SDK have no room for, so the state
 * after the pointer is copied into a BearSSL context around every call.
 */
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonCryptor.h>
#include <CommonCrypto/CommonKeyDerivation.h>
#include <CommonCrypto/CommonRandom.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bearssl.h"

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
CC_SHA256_Update(CC_SHA256_CTX *c, const void *data, CC_LONG len)
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

/* SHA-224: the SHA-256 core with other initial values, 28 bytes out */
int
CC_SHA224_Init(CC_SHA256_CTX *c)
{
	static const uint32_t iv[8] = { 0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4 };

	memcpy(c->hash, iv, sizeof(iv));
	c->count[0] = c->count[1] = 0;
	return 1;
}

int
CC_SHA224_Update(CC_SHA256_CTX *c, const void *data, CC_LONG len)
{
	return CC_SHA256_Update(c, data, len);
}

int
CC_SHA224_Final(unsigned char *md, CC_SHA256_CTX *c)
{
	unsigned char full[32];

	CC_SHA256_Final(full, c);
	memcpy(md, full, 28);
	return 1;
}

unsigned char *
CC_SHA224(const void *data, CC_LONG len, unsigned char *md)
{
	CC_SHA256_CTX c;

	CC_SHA224_Init(&c);
	CC_SHA224_Update(&c, data, len);
	CC_SHA224_Final(md, &c);
	return md;
}

unsigned char *
CC_SHA256(const void *data, CC_LONG len, unsigned char *md)
{
	CC_SHA256_CTX c;

	CC_SHA256_Init(&c);
	CC_SHA256_Update(&c, data, len);
	CC_SHA256_Final(md, &c);
	return md;
}

/* ---- the BearSSL-backed digests ---- */

/* copy the state that follows the vtable pointer between a CC context and a BearSSL one */
#define STATE_OFF(t) offsetof(t, buf)
#define STATE_LEN(t) (sizeof(t) - offsetof(t, buf))

#define BR_DIGEST(NAME, CCNAME, BRTYPE, BRPREFIX, OUTLEN, CTXTYPE) \
_Static_assert(STATE_LEN(BRTYPE) <= sizeof(CTXTYPE), #NAME " state does not fit the SDK context"); \
int \
CCNAME##_Init(CTXTYPE *c) \
{ \
	BRTYPE b; \
	BRPREFIX##_init(&b); \
	memset(c, 0, sizeof(*c)); \
	memcpy(c, (char *)&b + STATE_OFF(BRTYPE), STATE_LEN(BRTYPE)); \
	return 1; \
} \
int \
CCNAME##_Update(CTXTYPE *c, const void *data, CC_LONG len) \
{ \
	BRTYPE b; \
	BRPREFIX##_init(&b); \
	memcpy((char *)&b + STATE_OFF(BRTYPE), c, STATE_LEN(BRTYPE)); \
	BRPREFIX##_update(&b, data, len); \
	memcpy(c, (char *)&b + STATE_OFF(BRTYPE), STATE_LEN(BRTYPE)); \
	return 1; \
} \
int \
CCNAME##_Final(unsigned char *md, CTXTYPE *c) \
{ \
	BRTYPE b; \
	BRPREFIX##_init(&b); \
	memcpy((char *)&b + STATE_OFF(BRTYPE), c, STATE_LEN(BRTYPE)); \
	BRPREFIX##_out(&b, md); \
	return 1; \
} \
unsigned char * \
CCNAME(const void *data, CC_LONG len, unsigned char *md) \
{ \
	BRTYPE b; \
	BRPREFIX##_init(&b); \
	BRPREFIX##_update(&b, data, len); \
	BRPREFIX##_out(&b, md); \
	return md; \
}

BR_DIGEST(md5, CC_MD5, br_md5_context, br_md5, 16, CC_MD5_CTX)
BR_DIGEST(sha1, CC_SHA1, br_sha1_context, br_sha1, 20, CC_SHA1_CTX)
BR_DIGEST(sha384, CC_SHA384, br_sha384_context, br_sha384, 48, CC_SHA512_CTX)
BR_DIGEST(sha512, CC_SHA512, br_sha512_context, br_sha512, 64, CC_SHA512_CTX)

/* ---- HMAC and PBKDF2 over a table of the digests ---- */

typedef struct {
	size_t block, out;
	void (*init)(void *);
	void (*update)(void *, const void *, size_t);
	void (*final)(void *, unsigned char *);
} hash_ops;

typedef union {
	CC_MD5_CTX md5;
	CC_SHA1_CTX sha1;
	CC_SHA256_CTX sha256;
	CC_SHA512_CTX sha512;
} any_ctx;

#define OPS(NAME, CTX, BLOCK, OUT, CCNAME) \
static void NAME##_i(void *c) { CCNAME##_Init((CTX *)c); } \
static void NAME##_u(void *c, const void *d, size_t n) { CCNAME##_Update((CTX *)c, d, (CC_LONG)n); } \
static void NAME##_f(void *c, unsigned char *o) { CCNAME##_Final(o, (CTX *)c); } \
static const hash_ops NAME##_ops = { BLOCK, OUT, NAME##_i, NAME##_u, NAME##_f };

OPS(md5, CC_MD5_CTX, 64, 16, CC_MD5)
OPS(sha1, CC_SHA1_CTX, 64, 20, CC_SHA1)
OPS(sha224, CC_SHA256_CTX, 64, 28, CC_SHA224)
OPS(sha256, CC_SHA256_CTX, 64, 32, CC_SHA256)
OPS(sha384, CC_SHA512_CTX, 128, 48, CC_SHA384)
OPS(sha512, CC_SHA512_CTX, 128, 64, CC_SHA512)

/* the state of a CCHmacContext: which digest, the key xor opad, the running inner digest */
typedef struct {
	const hash_ops *ops;
	unsigned char opad[128];
	any_ctx inner;
} hmac_state;

_Static_assert(sizeof(hmac_state) <= sizeof(CCHmacContext), "hmac state does not fit CCHmacContext");

static const hash_ops *
hmac_ops(CCHmacAlgorithm alg)
{
	switch (alg) {
	case kCCHmacAlgSHA1: return &sha1_ops;
	case kCCHmacAlgMD5: return &md5_ops;
	case kCCHmacAlgSHA256: return &sha256_ops;
	case kCCHmacAlgSHA384: return &sha384_ops;
	case kCCHmacAlgSHA512: return &sha512_ops;
	case kCCHmacAlgSHA224: return &sha224_ops;
	}
	return NULL;
}

static void
hmac_begin(hmac_state *h, const hash_ops *ops, const void *key, size_t keylen)
{
	unsigned char k[128] = { 0 }, ipad[128];
	any_ctx t;

	h->ops = ops;
	if (keylen > ops->block) {
		ops->init(&t);
		ops->update(&t, key, keylen);
		ops->final(&t, k);
	} else {
		memcpy(k, key, keylen);
	}
	for (size_t i = 0; i < ops->block; i++) {
		ipad[i] = k[i] ^ 0x36;
		h->opad[i] = k[i] ^ 0x5c;
	}
	ops->init(&h->inner);
	ops->update(&h->inner, ipad, ops->block);
}

static void
hmac_end(hmac_state *h, unsigned char *mac)
{
	const hash_ops *ops = h->ops;
	unsigned char ih[64];
	any_ctx o;

	ops->final(&h->inner, ih);
	ops->init(&o);
	ops->update(&o, h->opad, ops->block);
	ops->update(&o, ih, ops->out);
	ops->final(&o, mac);
}

void
CCHmacInit(CCHmacContext *ctx, CCHmacAlgorithm algorithm, const void *key, size_t keyLength)
{
	const hash_ops *ops = hmac_ops(algorithm);

	if (ops) {
		hmac_begin((hmac_state *)ctx, ops, key, keyLength);
	}
}

void
CCHmacUpdate(CCHmacContext *ctx, const void *data, size_t dataLength)
{
	hmac_state *h = (hmac_state *)ctx;

	h->ops->update(&h->inner, data, dataLength);
}

void
CCHmacFinal(CCHmacContext *ctx, void *macOut)
{
	hmac_end((hmac_state *)ctx, macOut);
}

void
CCHmac(CCHmacAlgorithm algorithm, const void *key, size_t keyLength, const void *data, size_t dataLength, void *macOut)
{
	hmac_state h;
	const hash_ops *ops = hmac_ops(algorithm);

	if (ops) {
		hmac_begin(&h, ops, key, keyLength);
		ops->update(&h.inner, data, dataLength);
		hmac_end(&h, macOut);
	}
}

int
CCKeyDerivationPBKDF(CCPBKDFAlgorithm algorithm, const char *password, size_t passwordLen, const uint8_t *salt, size_t saltLen,
    CCPseudoRandomAlgorithm prf, uint32_t rounds, uint8_t *derivedKey, size_t derivedKeyLen)
{
	CCHmacAlgorithm alg;
	const hash_ops *ops;
	uint32_t block = 1;

	switch (prf) {
	case kCCPRFHmacAlgSHA1: alg = kCCHmacAlgSHA1; break;
	case kCCPRFHmacAlgSHA224: alg = kCCHmacAlgSHA224; break;
	case kCCPRFHmacAlgSHA256: alg = kCCHmacAlgSHA256; break;
	case kCCPRFHmacAlgSHA384: alg = kCCHmacAlgSHA384; break;
	case kCCPRFHmacAlgSHA512: alg = kCCHmacAlgSHA512; break;
	default: return kCCParamError;
	}
	ops = hmac_ops(alg);
	if (algorithm != kCCPBKDF2 || rounds == 0 || derivedKey == NULL) {
		return kCCParamError;
	}
	while (derivedKeyLen > 0) {
		unsigned char u[64], t[64], be[4] = { block >> 24, block >> 16, block >> 8, block };
		hmac_state h;
		size_t n = derivedKeyLen < ops->out ? derivedKeyLen : ops->out;

		hmac_begin(&h, ops, password, passwordLen);
		ops->update(&h.inner, salt, saltLen);
		ops->update(&h.inner, be, 4);
		hmac_end(&h, u);
		memcpy(t, u, ops->out);
		for (uint32_t i = 1; i < rounds; i++) {
			hmac_begin(&h, ops, password, passwordLen);
			ops->update(&h.inner, u, ops->out);
			hmac_end(&h, u);
			for (size_t j = 0; j < ops->out; j++) {
				t[j] ^= u[j];
			}
		}
		memcpy(derivedKey, t, n);
		derivedKey += n;
		derivedKeyLen -= n;
		block++;
	}
	return kCCSuccess;
}

/* ---- AES: CCCrypt with CBC (default) or ECB, PKCS#7 padding optional ---- */

CCCryptorStatus
CCCrypt(CCOperation op, CCAlgorithm alg, CCOptions options, const void *key, size_t keyLength, const void *iv,
    const void *dataIn, size_t dataInLength, void *dataOut, size_t dataOutAvailable, size_t *dataOutMoved)
{
	size_t len = dataInLength, i;
	unsigned char *out = dataOut, zero[16] = { 0 };
	int pad = (options & kCCOptionPKCS7Padding) != 0, ecb = (options & kCCOptionECBMode) != 0;

	if (dataOutMoved) {
		*dataOutMoved = 0;
	}
	if (alg != kCCAlgorithmAES || (keyLength != 16 && keyLength != 24 && keyLength != 32) || key == NULL ||
	    (dataIn == NULL && dataInLength) || out == NULL || (op != kCCEncrypt && op != kCCDecrypt)) {
		return kCCParamError;
	}
	if (op == kCCEncrypt) {
		size_t padded = pad ? (len / 16 + 1) * 16 : len;
		unsigned char fill = (unsigned char)(padded - len);

		if (!pad && (len % 16)) {
			return kCCAlignmentError;
		}
		if (padded > dataOutAvailable) {
			return kCCBufferTooSmall;
		}
		memmove(out, dataIn, len);
		memset(out + len, fill, padded - len);
		br_aes_ct_cbcenc_keys k;
		br_aes_ct_cbcenc_init(&k, key, keyLength);
		if (ecb) {
			for (i = 0; i < padded; i += 16) {
				unsigned char z[16] = { 0 };

				br_aes_ct_cbcenc_run(&k, z, out + i, 16);
			}
		} else {
			unsigned char v[16];

			if (iv) {
				memcpy(v, iv, 16);
			} else {
				memset(v, 0, 16);
			}
			br_aes_ct_cbcenc_run(&k, v, out, padded);
		}
		if (dataOutMoved) {
			*dataOutMoved = padded;
		}
		return kCCSuccess;
	}
	if (len % 16 || (pad && len == 0)) {
		return kCCAlignmentError;
	}
	if (len > dataOutAvailable) {
		return kCCBufferTooSmall;
	}
	memmove(out, dataIn, len);
	{
		br_aes_ct_cbcdec_keys k;

		br_aes_ct_cbcdec_init(&k, key, keyLength);
		if (ecb) {
			for (i = 0; i < len; i += 16) {
				memset(zero, 0, 16);
				br_aes_ct_cbcdec_run(&k, zero, out + i, 16);
			}
		} else {
			unsigned char v[16];

			if (iv) {
				memcpy(v, iv, 16);
			} else {
				memset(v, 0, 16);
			}
			br_aes_ct_cbcdec_run(&k, v, out, len);
		}
	}
	if (pad) {
		unsigned char n = len ? out[len - 1] : 0;

		if (n == 0 || n > 16 || n > len) {
			return kCCDecodeError;
		}
		for (i = len - n; i < len; i++) {
			if (out[i] != n) {
				return kCCDecodeError;
			}
		}
		len -= n;
	}
	if (dataOutMoved) {
		*dataOutMoved = len;
	}
	return kCCSuccess;
}

CCRNGStatus
CCRandomGenerateBytes(void *bytes, size_t count)
{
	arc4random_buf(bytes, count);
	return kCCSuccess;
}
