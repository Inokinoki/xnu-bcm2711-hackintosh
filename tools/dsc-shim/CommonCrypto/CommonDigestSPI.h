/* The part of CommonCrypto's private CommonDigestSPI.h that dyld's cache builder uses, on top of the public API. */
#ifndef DSC_SHIM_COMMONDIGESTSPI_H
#define DSC_SHIM_COMMONDIGESTSPI_H
#include <stdint.h>
#include <stddef.h>
#include <CommonCrypto/CommonDigest.h>

typedef uint32_t CCDigestAlg;
enum { kCCDigestNone = 0, kCCDigestSHA1 = 8, kCCDigestSHA256 = 10, kCCDigestSHA384 = 11, kCCDigestSHA512 = 12 };

static inline int CCDigest(CCDigestAlg alg, const uint8_t* data, size_t len, uint8_t* out)
{
    switch ( alg ) {
        case kCCDigestSHA1:   CC_SHA1(data, (CC_LONG)len, out);   return 0;
        case kCCDigestSHA256: CC_SHA256(data, (CC_LONG)len, out); return 0;
        case kCCDigestSHA384: CC_SHA384(data, (CC_LONG)len, out); return 0;
        case kCCDigestSHA512: CC_SHA512(data, (CC_LONG)len, out); return 0;
    }
    return -1;
}
#endif
