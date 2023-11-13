cmake_minimum_required(VERSION 3.12)


# Note: need pass CMAKE_TOOLCHAIN_FILE refer to vcpkg in cmdline
set(CMAKE_TOOLCHAIN_FILE /home/gyl/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "" FORCE)
set(VCPKG_BINARY_CACHE ON CACHE BOOL "" FORCE)

project(feature_jidl_test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# enable asan
add_definitions("-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")

# include framework cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt)

# add test executable
add_executable(${PROJECT_NAME}
    simple_1_0.cpp
    simple_1_0_impl.cpp
    record_1_0.cpp
    record_1_0_impl.cpp
    struct_test.cpp
    struct_test_impl.cpp
    promise_test.cpp
    promise_test_impl.cpp
    interface_test.cpp
    interface_test_impl.cpp
    any_test.cpp
    any_test_impl.cpp
    arraybuffer_test.cpp
    arraybuffer_test_impl.cpp
    crypto_native.cpp
    crypto_utils.cpp
    crypto.cpp
    crypto_impl.cpp
    cypher.cpp
    cypher_impl.cpp
    jidl_main.cpp
    feature_timers.cpp
    feature_interface.cpp
    ajs_features_list.h
    ajs_features_init.h
)

set(GTEST_DIR ${PROJECT_SOURCE_DIR}/../../../../../external/googletest/googletest/googletest)

set(GTEST_SRC ${GTEST_DIR}/src/gtest-all.cc)

# add test executable
include_directories(${GTEST_DIR}/include
                    ${GTEST_DIR})
add_executable("feat_test"
    ${GTEST_SRC}
    feat_test.h
    feat_test_impl.cpp
    feat_test.cpp
    test_main.cpp
    ajs_features_list.h
    ajs_features_init.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    feature-framework
    uv
)

target_link_libraries("feat_test" PRIVATE
    feature-framework
    pthread
    uv

)
