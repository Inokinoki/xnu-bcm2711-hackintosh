/*
 * os/feature_private.h comes with libsystem_featureflags, which is closed. There is no feature-flag
 * store on this system: every flag has its compiled-in default.
 */
#ifndef __OS_FEATURE_PRIVATE_H__
#define __OS_FEATURE_PRIVATE_H__

#include <stdbool.h>

#define os_feature_enabled(domain, feature) (false)
#define os_feature_enabled_simple(domain, feature, default_value) (default_value)

#endif /* __OS_FEATURE_PRIVATE_H__ */
