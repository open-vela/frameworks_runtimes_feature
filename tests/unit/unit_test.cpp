#include "gtest/gtest.h"

#include "unit_util.h"

#include "unit_test_context.cpp"
#include "unit_test_export.cpp"
#include "unit_test_main_export.cpp"
#include "unit_test_qjs_exports.cpp"

namespace feature_framework {

extern "C" int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace feature_framework
