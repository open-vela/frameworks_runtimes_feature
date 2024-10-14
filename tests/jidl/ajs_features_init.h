#include "feature_exports.h"

#undef QAPPFEATURE_INIT
#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(FeatureRegistryHandle handle)

// bool jse_interface_initFeature(FeatureRegistryHandle handle);
bool jse_Simple_initFeature(FeatureRegistryHandle handle);
// bool jse_Record_initFeature(FeatureRegistryHandle handle);
// bool jse_struct_test_initFeature(FeatureRegistryHandle handle);
bool jse_promise_test_initFeature(FeatureRegistryHandle handle);
bool jse_interface_test_initFeature(FeatureRegistryHandle handle);
// bool jse_mockatest_initFeature(FeatureRegistryHandle handle);
// bool jse_device_initFeature(FeatureRegistryHandle handle);
// bool jse_feat_test_initFeature(FeatureRegistryHandle handle);

#ifdef __cplusplus
extern "C" {
#endif
bool jse_struct_test_initFeature(FeatureRegistryHandle handle);
#ifdef __cplusplus
}
#endif
