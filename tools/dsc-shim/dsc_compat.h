/* Force-included into the C++ sources of dyld's cache builder (dyld-cache workflow). */
#ifndef DSC_COMPAT_H
#define DSC_COMPAT_H
#ifdef __cplusplus
#include <set>
#include <string>
/* An Apple-internal list of weak dylibs that may be absent from a build; ours is empty. */
static const std::set<std::string> allowedMissingWeakDylibs;
#endif
#endif
