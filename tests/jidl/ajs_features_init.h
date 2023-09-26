#include "feature_registry.h"

using namespace ferry;
#undef QAPPFEATURE_INIT
#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features)

bool jse_timers_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_interface_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Simple_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Record_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Struct_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Promise_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_ATest_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
bool jse_Interface_initFeature(ferry::FeatureRegistry *mgr, std::vector<std::string>&features);
