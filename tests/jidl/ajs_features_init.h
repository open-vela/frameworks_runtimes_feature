#include "feature_registry.h"

using namespace ferry;
#undef QAPPFEATURE_INIT
#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features)

bool jse_timers_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_interface_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Simple_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Record_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_struct_test_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_promise_test_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_ATest_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_interface_test_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
// bool jse_mockatest_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
