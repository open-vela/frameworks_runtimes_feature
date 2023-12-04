cmake_minimum_required(VERSION 3.12)


# Note: need pass CMAKE_TOOLCHAIN_FILE refer to vcpkg in cmdline
set(CMAKE_TOOLCHAIN_FILE /home/lijianbin/installed/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "" FORCE)
set(VCPKG_BINARY_CACHE ON CACHE BOOL "" FORCE)

project(feature_jidl_test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set (WAMR_BUILD_STRINGREF 1)

# enable asan
add_definitions("-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer  -g")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fpermissive -Wall")

set(UNITE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../../../apps/interpreters)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wall")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

## WAMR
include(${UNITE_PATH}/wamr/wamr/runtime-library/wamr_config.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
## quickjs
set(QUICKJS_SRC_DIR ${UNITE_PATH}/quickjs/quickjs)
include_directories(${QUICKJS_SRC_DIR})
set(QUICKJS_SOURCE
    ${QUICKJS_SRC_DIR}/cutils.c
    ${QUICKJS_SRC_DIR}/libregexp.c
    ${QUICKJS_SRC_DIR}/libunicode.c
    ${QUICKJS_SRC_DIR}/quickjs.c)

add_definitions(-DCONFIG_VERSION="20230815")
## libdyntype
set(LIBDYNTYPE_DIR ${UNITE_PATH}/wamr/wamr/runtime-library/libdyntype)
include (${LIBDYNTYPE_DIR}/libdyntype.cmake)

# stdlib
set(STDLIB_DIR ${UNITE_PATH}/wamr/wamr/runtime-library/stdlib)
include_directories(${STDLIB_DIR})
set(STDLIB_SOURCE
    ${STDLIB_DIR}/lib_console.c
    ${STDLIB_DIR}/lib_array.c
    ${STDLIB_DIR}/lib_timer.c
)
# struct-indirect
set(STRUCT_INDIRECT_DIR ${UNITE_PATH}/wamr/wamr/runtime-library/struct-indirect)
include_directories(${STRUCT_INDIRECT_DIR})
set(STRUCT_INDIRECT_SOURCE
    ${STRUCT_INDIRECT_DIR}/lib_struct_indirect.c
)
# utils
set(UTILS_DIR ${UNITE_PATH}/wamr/wamr/runtime-library/utils)
include_directories(${UTILS_DIR})
set(UTILS_SOURCE
    ${UTILS_DIR}/type_utils.c
)

set(OBJECT_UTILS_SOURCE
    ${UTILS_DIR}/object_utils.c
)

set(WAMR_UTILS_SOURCE
    ${UTILS_DIR}/wamr_utils.c
)
# feature demo 
set(FEATURE_DEMO_SOURCE
    ${CMAKE_CURRENT_LIST_DIR}/feature_demo.cpp
)

# include framework cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt)

# add test executable
add_executable(${PROJECT_NAME}
    simple_1_0.cpp
    simple_1_0_impl.cpp
    record_1_0.cpp
    record_1_0_impl.cpp
    ATest_1_0.cpp
    ATest_1_0_impl.cpp
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
    cipher.cpp
    cipher_impl.cpp
    jidl_main.cpp
    builtin/builtin_console.cpp
    feature_timers.cpp
    feature_interface.cpp
    ajs_features_list.h
    ajs_features_init.h
    ${LIBDYNTYPE_SRC}
    ${STDLIB_SOURCE}
    ${STRUCT_INDIRECT_SOURCE}
    ${UTILS_SOURCE}
    ${OBJECT_UTILS_SOURCE}
    ${WAMR_UTILS_SOURCE}
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
    builtin/builtin_console.cpp
    ajs_features_list.h
    ajs_features_init.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    feature-framework
    uv
    vmlib
    -lpthread
)

target_link_libraries("feat_test" PRIVATE
    feature-framework
    pthread
    uv

)
