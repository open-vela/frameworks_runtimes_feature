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
  if(CONFIG_FEATURE_LOG_LEVEL)
    list(APPEND FEATURE_DEFS -DFEATURE_LOG_LEVEL=${CONFIG_FEATURE_LOG_LEVEL})
  endif()

  list(
    APPEND
    FEATURE_SRCS
    ${FEATURE_SRC_DIR}/feature_common.cpp
    ${FEATURE_SRC_DIR}/feature_context.cpp
    ${FEATURE_SRC_DIR}/feature_context_qjs.cpp
    ${FEATURE_SRC_DIR}/feature_exports.cpp
    ${FEATURE_SRC_DIR}/feature_ffi.cpp
    ${FEATURE_SRC_DIR}/feature_ffi_qjs.cpp
    ${FEATURE_SRC_DIR}/feature_instance.cpp
    ${FEATURE_SRC_DIR}/feature_instance_qjs.cpp
    ${FEATURE_SRC_DIR}/feature_manager.cpp
    ${FEATURE_SRC_DIR}/feature_manager_qjs.cpp
    ${FEATURE_SRC_DIR}/feature_prototype.cpp
    ${FEATURE_SRC_DIR}/feature_prototype_qjs.cpp
    ${FEATURE_SRC_DIR}/feature_object_ref.cpp
    ${FEATURE_SRC_DIR}/feature_registry.cpp
    ${FEATURE_SRC_DIR}/promise_manager.cpp
    ${FEATURE_SRC_DIR}/value_translator_qjs.cpp)

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
      ${FEATURE_SRC_DIR}/feature_context_wamr.cpp
      ${FEATURE_SRC_DIR}/feature_ffi_wamr.cpp
      ${FEATURE_SRC_DIR}/feature_instance_wamr.cpp
      ${FEATURE_SRC_DIR}/feature_prototype_wamr.cpp
      ${FEATURE_SRC_DIR}/feature_manager_wamr.cpp
      ${FEATURE_SRC_DIR}/feature_wamr_utils.cpp
      ${FEATURE_SRC_DIR}/value_translator_wamr.cpp)
  endif()

  # set some include directories
  list(
    APPEND
    FEATURE_INC_DIRS
    ${FEATURE_TOP}/include
    ${FEATURE_SRC_DIR}
    ${FEATURE_MODULES_DIR}
    ${RAPIDJSON_DIR}/include
    ${QUICKJS_DIR}
    ${QUICKAPP_DIR}/src)

  if(CONFIG_QUICKAPP_VAPP_XMS)
    list(APPEND
    FEATURE_INC_DIRS
    ${QUICKAPP_DIR}/src/framework
    ${QUICKAPP_DIR}/src/jse
    ${APP_DIR}/frameworks/runtimes/services/am/include)
  endif()

  list(APPEND FEATURE_OPTIONS -Wno-format)
  # workaround for gcc-13 warning
  list(APPEND FEATURE_OPTIONS --param=min-pagesize=0)

  # for the 'service_internal_account' feature
  if(CONFIG_ACCOUNT_FEATURE)
    list(APPEND FEATURELIST "service_internal_account")
  endif()

  # for the 'system_internal_micamera' feature
  if(CONFIG_MIJIA_CAMERA_CLIENT)
    list(APPEND FEATURELIST "system_internal_micamera")
  endif()

  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/app_path.cpp)

  # for the 'locale' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/locale_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/feature_locale.jidl)
  list(APPEND FEATURELIST "locale")

  # for the 'Error' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/error_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/error.jidl)
  list(APPEND FEATURELIST "Error")

  # for the 'system_device' feature compile in
  # quickapp/src/jse/modules/system/CMakeLists.txt
  list(APPEND FEATURELIST "system_device")

  # for the 'system_internal_test' feature
  if(CONFIG_QUICKAPP_TEST_FRAMEWORK)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/internal_test_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/internal_test.jidl)
    list(APPEND FEATURELIST "system_internal_test")
  endif()

  # for the 'system_fetch' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/net_utils.cpp)
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/fetch_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/fetch.jidl)
  list(APPEND FEATURELIST "system_fetch")

  # for the 'system_prompt' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/prompt_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/prompt.jidl)
  list(APPEND FEATURELIST "system_prompt")

  # for the 'system_uploadtask' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/upload_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/uploadtask.jidl)
  list(APPEND FEATURELIST "system_uploadtask")

  if(CONFIG_FEATURE_TEST_CLIENT)
    list(APPEND FEATURE_CLI_SRCS ${FEATURE_MODULES_DIR}/feature_test_cli.cpp)
    list(APPEND FEATURE_CLI_INC_DIRS ${FEATURE_TOP}/include ${FEATURE_SRC_DIR})

    nuttx_add_application(
      NAME
      feature_test_cli
      PRIORITY
      100
      STACKSIZE
      4096000
      DEPENDS
      libfeature
      SRCS
      ${FEATURE_CLI_SRCS}
      INCLUDE_DIRECTORIES
      ${FEATURE_CLI_INC_DIRS})
  endif()

  # for the 'jumpApp' feature compile in
  # quickapp/src/jse/modules/system/CMakeLists.txt
  list(APPEND FEATURELIST "jumpApp")

  if(CONFIG_QUICKAPP)
    # for the 'system_router' feature
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/router_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/router.jidl)
    list(APPEND FEATURELIST "system_router")

    # for the 'system_app' feature
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/system_app_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/system_app.jidl)
    list(APPEND FEATURELIST "system_app")
  endif()

  # for media features
  if(CONFIG_MEDIA_FEATURE)
    list(APPEND CFEATURELIST "system_volume")
    list(APPEND CFEATURELIST "system_audio")
    list(APPEND CFEATURELIST "system_media_session")
  endif()

  if(CONFIG_LIBUV_EXTENSION)
    # for the 'system_storage' feature
    if(CONFIG_UNQLITE)
      list(APPEND FEATURE_INC_DIRS ${EXTERNAL_DIR}/unqlite/unqlite)
      list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/storage.cpp
           ${FEATURE_MODULES_DIR}/storage_impl.cpp)
      list(APPEND FEATURELIST "system_storage")
    endif()

    # for the 'system_exchange' feature
    if(CONFIG_KVDB)
      list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/exchange_impl.cpp)
      list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/exchange.jidl)
      list(APPEND FEATURELIST "system_exchange")
    endif()

    # for the safe features
    if(CONFIG_CRYPTO_MBEDTLS)
      list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/crypto_native.cpp
           ${FEATURE_MODULES_DIR}/crypto_utils.cpp
           ${FEATURE_MODULES_DIR}/crypto_impl.cpp)
      list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/crypto.jidl)
      list(APPEND FEATURELIST "system_crypto")
      list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/cipher_impl.cpp)
      list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/cipher.jidl)
      list(APPEND FEATURELIST "system_cipher")
    endif()

    # for the 'system_configuration' feature
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/configuration_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/configuration.jidl)
    list(APPEND FEATURELIST "system_configuration")
  endif()

  # for the 'system_sensor' feature
  if(CONFIG_UORB)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/sensor_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/sensor.jidl)
    list(APPEND FEATURELIST "system_sensor")
  endif()

  # for the 'system_internal_power' feature
  if(CONFIG_BOARDCTL)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/power_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/power.jidl)
    list(APPEND FEATURELIST "system_internal_power")
  endif()

  # for the 'system_vibrator' feature
  if(CONFIG_VIBRATOR)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/vibrator_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/vibrator.jidl)
    list(APPEND FEATURELIST "system_vibrator")
  endif()

  # for the 'system_internal_package' feature
  if(CONFIG_SYSTEM_PACKAGE_SERVICE)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/package_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/package.jidl)
    list(APPEND FEATURELIST "system_internal_package")
  endif()

  # for the 'system_internal_activity' feature
  if(CONFIG_SYSTEM_ACTIVITY_SERVICE)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/activity_feature_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/activity.jidl)
    list(APPEND FEATURELIST "system_internal_activity")
  endif()

  # for the 'system_messageChannel' feature
  if(CONFIG_SYSTEM_ACTIVITY_SERVICE)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/message_transport.cpp
        ${FEATURE_MODULES_DIR}/message_channel_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/message_channel.jidl)
    list(APPEND FEATURELIST "system_messageChannel")
  endif()

  # for the 'array_null' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/array_null_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/array_null.jidl)
  list(APPEND FEATURELIST "array_null")

  # for the 'promise_callback' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/promise_callback_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/promise_callback.jidl)
  list(APPEND FEATURELIST "promise_callback")

  # for the 'event_test' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/event_test_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/event_test.jidl)
  list(APPEND FEATURELIST "event_test")

  # for the 'system_request' feature
  if(CONFIG_UTILS_CURL)
    list(APPEND FEATURE_INC_DIRS ${SYSTEM_DIR}/libuv/ext/include)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/request_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/request.jidl)
    list(APPEND FEATURELIST "system_request")
  endif()

  # for the 'system_file' feature
  list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/file_impl.cpp)
  list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/file.jidl)
  list(APPEND FEATURELIST "system_file")

  # for the 'system_folme' feature
  if(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER)
    list(APPEND FEATURELIST "system_folme")
  endif()

  # for the 'system_zip' feature
  if(CONFIG_LIB_ZLIB)
    list(APPEND FEATURE_INC_DIRS ${SYSTEM_DIR}/zlib/zlib
         ${SYSTEM_DIR}/zlib/zlib/contrib/minizip)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/zip_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/zip.jidl)
    list(APPEND FEATURELIST "system_zip")
  endif()

  # for the bluetooth features
  if(CONFIG_BLUETOOTH_FEATURE)
    list(APPEND CFEATURELIST "system_bluetooth")
    list(APPEND CFEATURELIST "system_bluetooth_bt")

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND CFEATURELIST "system_bluetooth_bt_a2dpsink")
    endif()
  endif()

  # for the 'system_brightness' feature
  if(CONFIG_SYSTEM_BRIGHTNESS_SERVICE)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/brightness_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/brightness.jidl)
    list(APPEND FEATURELIST "system_brightness")
  endif()

  # for the 'service_wechat' feature
  if(CONFIG_APP_WECHAT)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/wechat_impl.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/wechat.jidl)
    list(APPEND FEATURELIST "service_wechat")
  endif()

  # # for the 'service_miplay' feature
  # if(CONFIG_MIPLAY_QAPP)
  #   list(APPEND FEATURELIST "service_miplay")
  # endif()

  # for the 'system_event' feature
  if(CONFIG_UORB)
    list(APPEND FEATURE_INC_DIRS ${SYSTEM_DIR}/topics/include/system)
    list(APPEND FEATURE_SRCS ${FEATURE_MODULES_DIR}/event_impl.cpp
         ${FEATURE_MODULES_DIR}/event/topic.cpp
         ${FEATURE_MODULES_DIR}/event/event_context.cpp
         ${FEATURE_MODULES_DIR}/event/topics/user_topic.cpp
         ${FEATURE_MODULES_DIR}/event/topics/battery_topic.cpp)
    list(APPEND JIDL_PATHS ${FEATURE_MODULES_DIR}/jidl/event.jidl)
    list(APPEND FEATURELIST "system_event")
  endif()

  if(CONFIG_MIWEAR_COMMON)
    list(APPEND FEATURE_INC_DIRS ${CMAKE_SOURCE_DIR}/../vendor/xiaomi/miwear/common/base/include)
  endif()

  if(CONFIG_LYRA_NEW_FEATURE)
    if(CONFIG_LYRA_NEW_FEATURE_TRANSFER)
      list(APPEND FEATURELIST "system_internal_hyperchannel")
    endif()
    if(CONFIG_LYRA_NEW_FEATURE_MESSAGE_CENTER)
      list(APPEND FEATURELIST "system_internal_messagecenter")
    endif()
    if(CONFIG_LYRA_NEW_FEATURE_MESSAGE_CENTER)
      list(APPEND FEATURELIST "system_internal_networking")
    endif()
  endif()

  if(CONFIG_SERVICE_AGENT_CLIENT)
    execute_process(
      COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/service_agent_features.sh
      OUTPUT_VARIABLE service_agent_features
      WORKING_DIRECTORY ${NUTTX_APPS_DIR}
    )
    list(APPEND FEATURELIST ${service_agent_features})
  endif()

  # generate pdat files list
  file(GLOB PDATLIST "registry/*.pdat")

  # generate ajs_features_list.h and ajs_features_registry.cpp
  set(AJS_REGISTRY_CONTENT
      [=[
#include "ajs_features_registry.h"
#include "ajs_features_list.h"

]=])
  set(AJS_REGISTRY_CONTENT
      "${AJS_REGISTRY_CONTENT}bool registerAjsFeatures(FeatureRegistryHandle handle) \{"
  )

  foreach(FEATURE_ITEM ${FEATURELIST})
    set(AJS_CALL "    if (!jse_${FEATURE_ITEM}_initFeature(handle))")
    set(AJS_CALL "${AJS_CALL}\n        return false\;")
    set(AJS_REGISTRY_CONTENT "${AJS_REGISTRY_CONTENT}\n${AJS_CALL}")
  endforeach()
  foreach(FEATURE_ITEM ${CFEATURELIST})
    set(AJS_CALL "    if (!jse_${FEATURE_ITEM}_initFeature(handle))")
    set(AJS_CALL "${AJS_CALL}\n        return false\;")
    set(AJS_REGISTRY_CONTENT "${AJS_REGISTRY_CONTENT}\n${AJS_CALL}")
  endforeach()
  set(AJS_REGISTRY_CONTENT "${AJS_REGISTRY_CONTENT}\n    return true\;\n\}")

  file(WRITE ${BINARY_SRC_DIR}/ajs_features_registry.cpp
       ${AJS_REGISTRY_CONTENT})
  list(APPEND FEATURE_SRCS ${BINARY_SRC_DIR}/ajs_features_registry.cpp)

  # generate ajs_features_init.h and ajs_features_list.h
  set(AJS_LIST_CONTENT
      [=[
#ifndef AJS_FEATURES_LIST_H_
#define AJS_FEATURES_LIST_H_

#include "feature_exports.h"
]=])

  foreach(FEATURE_ITEM ${FEATURELIST})
    set(FEATURE_DEF
        "bool jse_${FEATURE_ITEM}_initFeature(FeatureRegistryHandle handle);")
    set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n${FEATURE_DEF}")
  endforeach()

  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n\n#ifdef __cplusplus")
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\nextern \"C\" \{")
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n#endif")
  foreach(FEATURE_ITEM ${CFEATURELIST})
    set(FEATURE_DEF
        "bool jse_${FEATURE_ITEM}_initFeature(FeatureRegistryHandle handle);")
    set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n${FEATURE_DEF}")
  endforeach()
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n#ifdef __cplusplus")
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n\}")
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n#endif")
  set(AJS_LIST_CONTENT "${AJS_LIST_CONTENT}\n#endif")
  file(WRITE "${BINARY_SRC_DIR}/ajs_features_list.h" "${AJS_LIST_CONTENT}")
  list(APPEND FEATURE_INC_DIRS ${BINARY_SRC_DIR})

  set(FEATURE_EXPORTS
      ${FEATURE_TOP}/include/feature_types.h
      ${FEATURE_TOP}/include/feature_context.h
      ${FEATURE_TOP}/include/feature_description.h
      ${FEATURE_TOP}/include/feature_exports.h
      ${FEATURE_TOP}/include/feature_main_exports.h
      ${FEATURE_TOP}/include/feature_log.h
      ${FEATURE_TOP}/include/ajs_features_init.h
      ${FEATURE_TOP}/src/README.md
      ${FEATURE_TOP}/registry/README.md)

  if(NOT CONFIG_FEATURE_FRAMEWORK_NONE)
    set(FEATURE_PRIORITY ${CONFIG_FEATURE_FRAMEWORK_PRIORITY})
    set(FEATURE_STACKSIZE ${CONFIG_FEATURE_FRAMEWORK_STACKSIZE})
    set(FEATURE_MODULE ${CONFIG_FEATURE_FRAMEWORK})
  endif()

  nuttx_add_library(libfeature)
  target_sources(libfeature PRIVATE ${FEATURE_SRCS})
  nuttx_add_jidl(
    TARGET
    libfeature
    JIDL_SCRIPT
    ${PYTHON_SCRIPT}
    JIDL_OUT_DIR
    ${BINARY_MODULES_DIR}/src
    JIDLS
    ${JIDL_PATHS})
  target_include_directories(libfeature PRIVATE ${FEATURE_INC_DIRS})
  target_compile_options(libfeature PRIVATE ${FEATURE_OPTIONS})
  target_compile_definitions(libfeature PRIVATE ${FEATURE_DEFS})

  nuttx_export_header(TARGET libfeature INCLUDE_DIRECTORIES ${FEATURE_EXPORTS})
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${FEATURE_TOP} ${FEATURE_TOP}/include)

  # if(CONFIG_LIB_GOOGLETEST)
  #   list(APPEND FEATURE_TEST_SRCS ${FEATURE_TEST_DIR}/jidl/test_main.cpp)
  #   list(APPEND FEATURE_TEST_INC_DIRS ${FEATURE_TOP}/include ${FEATURE_SRC_DIR}
  #        ${GTEST_DIR}/include)

  #   nuttx_add_application(
  #     NAME
  #     feat_test
  #     PRIORITY
  #     100
  #     STACKSIZE
  #     4096
  #     DEPENDS
  #     libfeature
  #     SRCS
  #     ${FEATURE_TEST_SRCS}
  #     INCLUDE_DIRECTORIES
  #     ${FEATURE_TEST_INC_DIRS})
  # endif()

  if(CONFIG_FEATURE_UNIT_TEST)
    list(APPEND UNIT_TEST_SRCS ${FEATURE_TOP}/tests/unit/unit_test.cpp)
    list(APPEND UNIT_TEST_INC_DIRS ${FEATURE_TOP}/include ${FEATURE_SRC_DIR}
         ${GTEST_DIR}/include)

    nuttx_add_application(
      NAME
      unit_test
      PRIORITY
      100
      STACKSIZE
      4096
      DEPENDS
      libfeature
      SRCS
      ${UNIT_TEST_SRCS}
      INCLUDE_DIRECTORIES
      ${UNIT_TEST_INC_DIRS})
  endif()

  if(CONFIG_SYSTEM_ACTIVITY_SERVICE)
    # handle AIDL related stuffs
    file(GLOB_RECURSE AS_AIDL_PATHS ${FEATURE_MODULES_DIR}/aidl/*.aidl)
    nuttx_add_aidl(
      TARGET
      libfeature
      AIDL_BASE_DIR
      ${FEATURE_MODULES_DIR}/aidl
      AIDL_INCLUDE_DIR
      ${FEATURE_MODULES_DIR}/aidl
      AIDL_HEADER_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/aidl
      AIDL_OUT_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/aidl
      AIDL_FLAGS
      --lang=cpp
      AIDLS
      ${AS_AIDL_PATHS})
  endif()
endif()
