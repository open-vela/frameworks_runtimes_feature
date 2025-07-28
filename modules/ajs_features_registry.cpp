// clang-format off
#include "feature_exports.h"
#include "feature_description.h"

#include "features_registry_list.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include "cfeatures_registry_list.h"
#ifdef __cplusplus
}
#endif //__cplusplus

FeatureRegistryTable g_ajs_features_registry_table = {
    .count = 0,
    .data = {
#include "features_registry_table.h"
        nullptr
    }
};
FeatureRegistryTableHandle g_ajs_features_registry = &g_ajs_features_registry_table;
// clang-format on