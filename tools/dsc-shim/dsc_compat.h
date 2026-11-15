/* Force-included into the C++ sources of dyld's cache builder (dyld-cache workflow). */
#ifndef DSC_COMPAT_H
#define DSC_COMPAT_H
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/attr.h>
#ifdef __cplusplus
extern "C" {
#endif
/* private <sys/fsgetpath.h>; the function itself is in libSystem */
int openbyid_np(fsid_t *fsid, fsobj_id_t *objid, int flags);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
#include <set>
#include <string>
/* An Apple-internal list of weak dylibs that may be absent from a build; ours is empty. */
static const std::set<std::string> allowedMissingWeakDylibs;
#endif
#endif
