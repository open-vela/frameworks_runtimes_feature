cmake_minimum_required(VERSION 3.12)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/deps/vcpkg/scripts/buildsystems/vcpkg.cmake)




project(feature_unit_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# enable -Werror
add_definitions(-Werror)
find_package(protobuf-c CONFIG REQUIRED)

################### nuttx function definations ###################
function(nuttx_add_library NAME)
    add_library(${NAME})
endfunction(nuttx_add_library NAME)
# nuttx_add_jidl
function(nuttx_add_jidl)

endfunction(nuttx_add_jidl)

# nuttx_export_header
function(nuttx_export_header)

endfunction(nuttx_export_header)

add_custom_target(nuttx)

####################################################################

# enable asan
add_definitions("-fno-omit-frame-pointer -fsanitize=address -fno-omit-frame-pointer -g")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fno-omit-frame-pointer -fsanitize=address -fno-omit-frame-pointer -fpermissive -Wall")

set(CURRENT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(RELATIVE_PATH "../../../../../apps/interpreters")
get_filename_component(UNITE_PATH ${RELATIVE_PATH} ABSOLUTE BASE_DIR ${CURRENT_PATH})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wall")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

## quickjs
set(QUICKJS_SRC_DIR ${UNITE_PATH}/quickjs)
set(QUICKJS_SOURCE
    ${QUICKJS_SRC_DIR}/quickjs/cutils.c
    ${QUICKJS_SRC_DIR}/quickjs/libregexp.c
    ${QUICKJS_SRC_DIR}/quickjs/libunicode.c
    ${QUICKJS_SRC_DIR}/quickjs/quickjs.c)
add_library(quickjs STATIC
    ${QUICKJS_SOURCE}
)
target_include_directories(quickjs PUBLIC
    ${QUICKJS_SRC_DIR}
)
add_definitions(-DCONFIG_VERSION="20230815")


# feature framework configs
set(CONFIG_FEATURE_FRAMEWORK ON)
set(DISABLE_SYSTEM_FEATURE ON)
set(CONFIG_FEATURE_LOG_LEVEL 1)

# include framework cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/
)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt)
# include jidl.cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tools/cmake/jidl.cmake)

target_link_libraries(libfeature PUBLIC quickjs)

# add feature list
set(FEATURE_LIST_CPP "Struct" "Protobuf" "Interface")

set(FEATURE_LIST_C "")
jidl_codegen_registry(
    FEATURE_LIST_CPP ${FEATURE_LIST_CPP}
    FEATURE_LIST_C ${FEATURE_LIST_C}
)
# collect jidl files
file(GLOB JIDL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/jidl/*.jidl)
# generate jidl cpp wrapper files
jidl_codegen_files(
    JIDL_TOOL_PATH ${CMAKE_CURRENT_LIST_DIR}/../../tools/jidl
    OUT_PATH ${CMAKE_BINARY_DIR}
    JIDL_FILES_CPP ${JIDL_FILES}
    LANG c++
    # JIDL_FILES_C ${JIDL_FILES}
)

file(GLOB BUILDIN_SOURCES CONFIGURE_DEPENDS ./builtin/*.cpp ./builtin/*.h)
file(GLOB SRC_SOURCES CONFIGURE_DEPENDS ./src/*.cpp ./src/*.h ./src/*.c)
file(GLOB GENERATED_FILES CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/*.cpp ${CMAKE_BINARY_DIR}/*.c ${CMAKE_BINARY_DIR}/*.h)

set(PROTBUF_FILES
    ${CMAKE_SOURCE_DIR}/proto/test.proto
)

set(PROTOBUF_C_FILES "")
foreach(PROTOFILE ${PROTBUF_FILES})
    get_filename_component(FILE_NAME ${PROTOFILE} NAME_WE)
    list(APPEND PROTOBUF_C_FILES ${CMAKE_BINARY_DIR}/${FILE_NAME}.pb-c.c)
endforeach()
# # get protoc compiler path
# set(PROTOBUFC_EXECUTABLE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/protobuf/protoc")
# add_custom_command(OUTPUT ${PROTOBUF_C_FILES}
#     COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/generated
#     COMMAND ${PROTOBUFC_EXECUTABLE} -I${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTBUF_FILES} --c_out=${CMAKE_CURRENT_BINARY_DIR}/generated
#     DEPENDS ${PROTBUF_FILES}
# )

# add test executable
add_executable(${PROJECT_NAME}
    ${SRC_SOURCES}
    ${GENERATED_FILES}
    ${BUILDIN_SOURCES}
    ${JIDL_GENERATED_CPP_FILES}
    ${JIDL_GENERATED_C_FILES}
    ${JIDL_GENERATED_FEATURE_REGISTERY_FILES}
    # ${PROTOBUF_C_FILES}
)

target_link_libraries(libfeature PUBLIC
    protobuf-c::protobuf-c
)
add_executable(test_main
    tests/test_main.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_include_directories(test_main PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libfeature
    uv
    pthread
)

target_link_libraries(test_main PRIVATE
    libfeature
    uv
    pthread
    protobuf-c::protobuf-c
)



add_executable(unit_test 
    ${CMAKE_CURRENT_SOURCE_DIR}/../unit/unit_test.cpp
)

target_include_directories(unit_test PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

find_package(GTest CONFIG REQUIRED)

target_link_libraries(unit_test PRIVATE
    libfeature
    uv
    GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
)

