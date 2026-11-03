/*
 * vproc_priv.h belongs to launchd's liblaunch (closed since 10.10). dyld asks whether the process is managed by
 * launchd; the call itself is provided by our liblaunch/libxpc stand-in, only its declaration is needed here.
 */
#ifndef __VPROC_PRIV_H__
#define __VPROC_PRIV_H__

#include <stdint.h>

typedef void *vproc_t;
typedef void *vproc_err_t;
typedef unsigned int vproc_gsk_t;

#define VPROC_GSK_IS_MANAGED 3

#ifdef __cplusplus
extern "C" {
#endif
vproc_err_t vproc_swap_integer(vproc_t vp, vproc_gsk_t key, int64_t *inval, int64_t *outval);
#ifdef __cplusplus
}
#endif

#endif /* __VPROC_PRIV_H__ */
