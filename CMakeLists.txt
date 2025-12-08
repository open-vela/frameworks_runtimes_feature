# set the C++ file extension and standard
set(CXXEXT .cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FEATURE_TOP ${CMAKE_CURRENT_SOURCE_DIR})
set(FEATURE_SRC_DIR ${FEATURE_TOP}/src)
set(FEATURE_MODULES_DIR ${FEATURE_TOP}/modules)
set(FEATURE_TEST_DIR ${FEATURE_TOP}/tests)

set(APP_DIR ${CMAKE_SOURCE_DIR}/../apps)
set(SYSTEM_DIR ${APP_DIR}/system)
set(EXTERNAL_DIR ${APP_DIR}/external)
set(FRAMEWORKS_DIR ${APP_DIR}/frameworks)
set(WAMR_DIR ${APP_DIR}/interpreters/wamr/wamr)
set(QUICKJS_DIR ${APP_DIR}/interpreters/quickjs)
set(QUICKAPP_DIR ${FRAMEWORKS_DIR}/runtimes/quickapp)
set(TS2WASM_RUNTIME_DIR ${FRAMEWORKS_DIR}/typescript/ts2wasm/runtime-library)
set(RAPIDJSON_DIR ${EXTERNAL_DIR}/rapidjson/rapidjson)
set(GTEST_DIR ${EXTERNAL_DIR}/googletest/googletest/googletest)
set(PYTHON_SCRIPT ${FEATURE_TOP}/tools/jidl/jsongensource.py)
set(BINARY_MODULES_DIR ${CMAKE_CURRENT_BINARY_DIR}/modules)
set(BINARY_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

if(CONFIG_FEATURE_FRAMEWORK)

  if(CONFIG_FEATURE_RUST_MODULES)
    # include rust subdirectories
    add_subdirectory(rust)
  endif(CONFIG_FEATURE_RUST_MODULES)

  nuttx_add_prebuilt(
    BIN_TARGETS
    libfeature
    EXPORT_FILES
    include/feature_types.h
    include/feature_context.h
    include/feature_description.h
    include/feature_exports.h
    include/feature_main_exports.h
    include/feature_log.h
    include/feature_permission.h
    include/ajs_features_init.h
    src/README.md
    registry/README.md
    tools/jidl)

  if(CONFIG_FEATURE_LOG_LEVEL)
    list(APPEND FEATURE_DEFS -DFEATURE_LOG_LEVEL=${CONFIG_FEATURE_LOG_LEVEL})
  endif()
  nuttx_include_directories_for_all_apps(${CMAKE_CURRENT_SOURCE_DIR}/include)
  list(
    APPEND
    FEATURE_SRCS
    ${FEATURE_SRC_DIR}/feature_common.cpp
    ${FEATURE_SRC_DIR}/feature_context.cpp
    ${FEATURE_SRC_DIR}/feature_exports.cpp
    ${FEATURE_SRC_DIR}/feature_main_exports.cpp
    ${FEATURE_SRC_DIR}/feature_ffi.cpp
    ${FEATURE_SRC_DIR}/feature_instance.cpp
    ${FEATURE_SRC_DIR}/feature_manager.cpp
    ${FEATURE_SRC_DIR}/feature_prototype.cpp
    ${FEATURE_SRC_DIR}/feature_object_ref.cpp
    ${FEATURE_SRC_DIR}/feature_registry.cpp
    ${FEATURE_SRC_DIR}/feature_permission.cpp
    ${FEATURE_SRC_DIR}/permissions_manager.cpp
    ${FEATURE_SRC_DIR}/promise_manager.cpp
    ${FEATURE_SRC_DIR}/protobuf/proto_reflection.cpp
    ${FEATURE_SRC_DIR}/protobuf/proto_utils.cpp
    ${FEATURE_SRC_DIR}/worker_manager.cpp
    ${FEATURE_SRC_DIR}/framework_log.cpp)

  list(
    APPEND
    FEATURE_SRCS
    ${FEATURE_SRC_DIR}/backend/qjs/feature_context_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/feature_ffi_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/feature_instance_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/feature_manager_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/feature_prototype_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/value_translator_qjs.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/feature_qjs_exports.cpp
    ${FEATURE_SRC_DIR}/backend/qjs/array_buffer_qjs.cpp)

  if(CONFIG_FEATURE_ENABLE_TRACKER)
    list(APPEND FEATURE_SRCS ${FEATURE_SRC_DIR}/feature_tracker.cpp)
  endif(CONFIG_FEATURE_ENABLE_TRACKER)

  if(CONFIG_FEATURE_USE_WAMR)
    list(APPEND FEATURE_DEFS -DWASM_ENABLE_GC=1 -DWASM_ENABLE_STRINGREF=1
         -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0 -D__STDC_VERSION__=0)

    list(
      APPEND
      FEATURE_INC_DIRS
      ${WAMR_DIR}/core
      ${WAMR_DIR}/core/iwasm
      ${WAMR_DIR}/core/iwasm/include
      ${WAMR_DIR}/core/iwasm/interpreter
      ${WAMR_DIR}/core/iwasm/common/gc
      ${WAMR_DIR}/core/shared/utils
      ${WAMR_DIR}/core/shared/platform/nuttx
      ${TS2WASM_RUNTIME_DIR}/libdyntype
      ${TS2WASM_RUNTIME_DIR}/utils)

    list(
      APPEND
      FEATURE_SRCS
      ${FEATURE_SRC_DIR}/backend/wamr/feature_context_wamr.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/feature_ffi_wamr.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/feature_instance_wamr.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/feature_prototype_wamr.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/feature_manager_wamr.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/feature_wamr_utils.cpp
      ${FEATURE_SRC_DIR}/backend/wamr/value_translator_wamr.cpp)
  endif()

  # set some include directories
  list(
    APPEND
    FEATURE_INC_DIRS
    ${BINARY_MODULES_DIR}
    ${FEATURE_TOP}/include
    ${FEATURE_SRC_DIR}
    ${FEATURE_MODULES_DIR}
    ${RAPIDJSON_DIR}/include
    ${QUICKJS_DIR}
    ${QUICKAPP_DIR}/src
    ${EXTERNAL_DIR}/protobuf-c/protobuf-c
    ${QUICKAPP_DIR}/inspector/include)

  if(CONFIG_QUICKAPP_VAPP_XMS)
    list(APPEND FEATURE_INC_DIRS ${QUICKAPP_DIR}/src/framework
         ${QUICKAPP_DIR}/src/jse
         ${APP_DIR}/frameworks/runtimes/services/am/include)
  endif()

  list(APPEND FEATURE_OPTIONS -Wno-format)
  # workaround for gcc-13 warning
  list(APPEND FEATURE_OPTIONS --param=min-pagesize=0)

  set(FEATURE_EXPORTS
      ${FEATURE_TOP}/include/feature_types.h
      ${FEATURE_TOP}/include/feature_context.h
      ${FEATURE_TOP}/include/feature_description.h
      ${FEATURE_TOP}/include/feature_exports.h
      ${FEATURE_TOP}/include/feature_main_exports.h
      ${FEATURE_TOP}/include/feature_log.h
      ${FEATURE_TOP}/include/ajs_features_init.h
      ${FEATURE_TOP}/src/README.md)

  if(NOT CONFIG_FEATURE_FRAMEWORK_NONE)
    set(FEATURE_PRIORITY ${CONFIG_FEATURE_FRAMEWORK_PRIORITY})
    set(FEATURE_STACKSIZE ${CONFIG_FEATURE_FRAMEWORK_STACKSIZE})
    set(FEATURE_MODULE ${CONFIG_FEATURE_FRAMEWORK})
  endif()

  nuttx_library_ifndef(SKIP_PREBUILT libfeature)
  nuttx_sources(${FEATURE_SRCS})
  nuttx_include_directories(${FEATURE_INC_DIRS})
  nuttx_compile_options(${FEATURE_OPTIONS})
  nuttx_compile_definitions(${FEATURE_DEFS})

  nuttx_export_header(TARGET libfeature INCLUDE_DIRECTORIES ${FEATURE_EXPORTS})
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${FEATURE_TOP} ${FEATURE_TOP}/include)

  if(CONFIG_FEATURE_TEST_JSFILE)
    list(APPEND FEATURE_TEST_SRCS ${FEATURE_TEST_DIR}/jidl/test_main.cpp
         ${FEATURE_TEST_DIR}/jidl/builtin/builtin_console.cpp)
    list(
      APPEND
      FEATURE_TEST_INC_DIRS
      ${FEATURE_TOP}/include
      ${FEATURE_SRC_DIR}
      ${GTEST_DIR}/include
      ${QUICKAPP_DIR}/src
      ${QUICKAPP_DIR}/src/framework
      ${QUICKAPP_DIR}/src/jse
      ${QUICKAPP_DIR}/src/jse/quickjs
      ${QUICKAPP_DIR}/src/jse/modules
      ${QUICKAPP_DIR}/src/jse/modules/system
      ${CMAKE_BINARY_DIR}
      ${NUTTX_APPS_DIR}/external/protobuf-c/protobuf-c/
      ${NUTTX_APPS_DIR}/external/rapidjson/rapidjson/include/
      ${NUTTX_APPS_DIR}/frameworks/runtimes/ash/include)

    nuttx_add_application(
      NAME
      feat_test
      PRIORITY
      100
      STACKSIZE
      131072
      DEPENDS
      libfeature
      SRCS
      ${FEATURE_TEST_SRCS}
      INCLUDE_DIRECTORIES
      ${FEATURE_TEST_INC_DIRS})
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${EXTERNAL_DIR}/protobuf-c/protobuf-c)

  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/modules)

  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif()
