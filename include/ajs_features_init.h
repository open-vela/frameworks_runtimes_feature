#ifndef AJS_FEATURE_INIT_H_
#define AJS_FEATURE_INIT_H_

#include "feature_exports.h"

#undef QAPPFEATURE_INIT
#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(FeatureRegistryHandle handle)

#endif
