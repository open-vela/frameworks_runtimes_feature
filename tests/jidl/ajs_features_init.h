#include "feature_manager.h"

using namespace ferry;
#undef QAPPFEATURE_INIT
#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features)

bool jse_timers_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);
bool jse_Simple_1_0_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);
bool jse_Record_1_0_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);
bool jse_Struct_1_0_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);
bool jse_Promise_1_0_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);
