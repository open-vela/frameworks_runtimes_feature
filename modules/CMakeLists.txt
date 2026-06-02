#
# Copyright (C) 2025 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# add target
nuttx_add_library(feature_impls STATIC)
set(FEATURE_TOP ${NUTTX_APPS_DIR}/frameworks/runtimes/feature)
list(APPEND FEATURE_INCDIR ${FEATURE_TOP}/src ${FEATURE_TOP}/src/backend/qjs)
target_include_directories(
    feature_impls
    PRIVATE
    ${FEATURE_INCDIR}
    ${NUTTX_APPS_DIR}/frameworks/runtimes/ash/include
    ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/inspector/include)

# add some common srcs
target_sources(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/app_path.cpp)
target_sources(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/net_utils.cpp)

if(CONFIG_FEATURE_SYSTEM_CRYPTO OR CONFIG_FEATURE_SYSTEM_CIPHER)
    target_sources(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/crypto_native.cpp)
    target_sources(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/crypto_utils.cpp)
endif()

# set some include path
target_include_directories(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/framework/dom-protobuf)
target_include_directories(feature_impls PRIVATE ${CMAKE_BINARY_DIR}/jidl_generated)

if(CONFIG_FEATURE_SYSTEM_SENSOR)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/sensor_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/sensor.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_sensor)
endif()

if(CONFIG_FEATURE_SYSTEM_FILE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/file_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/file.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_file)
endif()

if(0) #CONFIG_FEATURE_SYSTEM_MEDIA) TODO change
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/media_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/media.jidl)
    target_include_directories(
        feature_impls
        PRIVATE
        ${NUTTX_APPS_DIR}/frameworks/graphics/animengine/include
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/include
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/shell/miwear
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/shell/vapp
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src
        ${CMAKE_BINARY_DIR}
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/gui
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/framework
        ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/jse/modules/system)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_media)
endif()

if(CONFIG_FEATURE_SYSTEM_REQUEST)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/request_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/request.jidl)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/system/libuv/libuv/include)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/../external/rapidjson/rapidjson/include)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/include)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_request)
endif()

if(CONFIG_FEATURE_SYSTEM_ZIP)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/zip_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/zip.jidl)
    target_include_directories(
        feature_impls
        PRIVATE
        ${NUTTX_APPS_DIR}/system/zlib/zlib
        ${NUTTX_APPS_DIR}/system/zlib/zlib/contrib/)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_zip)
endif()

if (CONFIG_FEATURE_SYSTEM_ZLIB)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/system_zlib_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/system_zlib.jidl)
    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_zlib)
endif()

if(CONFIG_FEATURE_SYSTEM_CRYPTO)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/crypto_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/crypto.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_crypto)
endif()

if(CONFIG_FEATURE_SYSTEM_MQTTMESSAGE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/mqttmessage_impl.cpp)
    list(APPEND FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/mqtt_message.pb-c.c)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/mqttmessage.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_mqttmessage)
endif()

if(CONFIG_FEATURE_SYSTEM_CIPHER)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/cipher_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/cipher.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_cipher)
endif()

if(CONFIG_FEATURE_SYSTEM_VIBRATOR)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/vibrator_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/vibrator.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_vibrator)
endif()

if(CONFIG_QUICKAPP_TEST_FRAMEWORK)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/internal_test_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/internal_test.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_internal_test)
endif()

if(CONFIG_FEATURE_SYSTEM_TEST_INPUT)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/input_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/input.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_test_input)
endif()

if(CONFIG_FEATURE_SYSTEM_INTERNAL_POWER)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/power_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/power.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_internal_power)
endif()

if(CONFIG_FEATURE_SYSTEM_INTERNAL_ACTIVITY)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/activity_feature_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/activity.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_internal_activity)
endif()

if(CONFIG_FEATURE_SYSTEM_INTERNAL_PACKAGE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/packagemanager_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/packagemanager.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_packagemanager)
endif()

if(CONFIG_FEATURE_SYSTEM_FETCH)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/fetch_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/fetch.jidl)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/include)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_fetch)
endif()

if(CONFIG_FEATURE_SYSTEM_PROMPT)
    set(FEATURE_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/prompt_impl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/font.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/prompt.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/prompt_dialog.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/prompt_toast.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/prompt_server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/prompt/prompt_utils.cpp
    )
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/prompt.jidl)
    target_include_directories(
        feature_impls
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/prompt
    )

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_prompt)
endif()

if(CONFIG_FEATURE_SYSTEM_MESSAGECHANNEL)
    set(AIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/aidl/message_transport/IMessageTransport.aidl
        ${CMAKE_CURRENT_LIST_DIR}/aidl/message_transport/IReply.aidl)
    nuttx_add_aidl(
        TARGET
        feature_impls
        AIDL_BASE_DIR
        ${CMAKE_CURRENT_LIST_DIR}/aidl
        AIDL_INCLUDE_DIR
        ${CMAKE_CURRENT_LIST_DIR}/aidl
        AIDL_HEADER_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/aidl
        AIDL_OUT_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/aidl
        AIDL_FLAGS
        --lang=cpp
        AIDLS
        ${AIDL_PATHS})

    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/message_channel_impl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/message_transport.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/message_channel.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_messageChannel)
endif()

if(CONFIG_FEATURE_SYSTEM_EXCHANGE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/exchange_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/exchange.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_exchange)
endif()

if(CONFIG_FEATURE_SYSTEM_CONFIGURATION)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/configuration_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/configuration.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_configuration)
endif()

if(CONFIG_FEATURE_SYSTEM_DEVICE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/device_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/device.jidl)

    if(CONFIG_QUICKAPP)
        target_include_directories(
            feature_impls
            PRIVATE
            ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src
            ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/jse/quickjs
            ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/src/jse)
    endif()

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_device)
endif()

if (CONFIG_FEATURE_SYSTEM_INTERCONNECT)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/interconnect_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/interconnect.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_interconnect)
endif()

if ((CONFIG_APP_CAR_CONTROL AND CONFIG_CAR_APP_SETTINGS) OR CONFIG_APP_CAR_SETTINGS_MOCK)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/system_settings_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/system_settings.jidl)
    if (CONFIG_APP_CAR_CONTROL AND CONFIG_CAR_APP_SETTINGS)
        target_include_directories(
            feature_impls
            PRIVATE
            ${NUTTX_APPS_DIR}/vendor/xiaomi/miwear/apps/applications/car_control/car_app_settings/
            ${NUTTX_APPS_DIR}/vendor/xiaomi/miwear/apps/applications/car_control/model/car_app/settings/
        )
    else() # CONFIG_APP_CAR_SETTINGS_MOCK
        list(APPEND FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/miwear_settings_mock.cpp)
    endif()

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_settings
    )
endif()

if(CONFIG_FEATURE_SYSTEM_BRIGHTNESS)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/brightness_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/brightness.jidl)

    # Add binder-related include paths when CONFIG_SYSTEM_BRIGHTNESS_SERVICE is enabled,
    # since miwear brightness headers do not include binder.
    if(CONFIG_SYSTEM_BRIGHTNESS_SERVICE)
        target_include_directories(feature_impls PRIVATE
            ${NUTTX_APPS_DIR}/frameworks/runtimes/services/brightness/include
            ${NUTTX_APPS_DIR}/external/android/frameworks/native/libs/binder/include)
    endif()

    target_include_directories(
        feature_impls
        PRIVATE
        ${NUTTX_APPS_DIR}/frameworks/runtimes/services/brightness/include
        ${NUTTX_APPS_DIR}/external/android/frameworks/native/libs/binder/include
        ${NUTTX_APPS_DIR}/external/android/system/core/libutils/include)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_brightness)
endif()

if(CONFIG_FEATURE_SYSTEM_EVENT)
    set(FEATURE_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/event_impl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/event/topic.cpp
        ${CMAKE_CURRENT_LIST_DIR}/event/event_context.cpp
        ${CMAKE_CURRENT_LIST_DIR}/event/topics/user_topic.cpp
        ${CMAKE_CURRENT_LIST_DIR}/event/topics/battery_topic.cpp)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/system/topics/include/system)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/event.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_event)
endif()

if(CONFIG_FEATURE_SYSTEM_EVENT_TEST)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/event_test_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/event_test.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_event_test)
endif()

if(CONFIG_FEATURE_SYSTEM_STORAGE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/storage_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/storage.jidl)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/external/unqlite/unqlite)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/include)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_storage)
endif()

if(CONFIG_FEATURE_SERVICE_WECHAT)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/wechat_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/wechat.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        service_wechat)
endif()

if(CONFIG_FEATURE_LOCALE)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/locale_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/feature_locale.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        locale)
endif()

if(CONFIG_FEATURE_ERROR)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/error_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/error.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        Error)
endif()

if(CONFIG_FEATURE_SYSTEM_BATTERY)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/battery_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/battery.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_battery)
endif()

if(CONFIG_FEATURE_SYSTEM_GEOLOCATION)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/geolocation_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/geolocation.jidl)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_geolocation)
endif()

if(CONFIG_FEATURE_SERVICE_HEALTH)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/health_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/health.jidl)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/netutils/cjson/cJSON)
    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        service_health)
endif()

if(CONFIG_FEATURE_INPUT_METHOD_CONTROLLER)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/imc_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/imc.jidl)
    target_include_directories(feature_impls PRIVATE ${NUTTX_APPS_DIR}/frameworks/runtimes/quickapp/shell/)
    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_imc)
endif()

if(CONFIG_AI_MODULE)
    set(FEATURE_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/ai/src/ai_speech_impl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ai/src/asr_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/ai/jidl/system_ai_speech.jidl)

    target_include_directories(
        feature_impls
        PRIVATE
        ${NUTTX_APPS_DIR}/netutils/cjson/cJSON
        ${NUTTX_APPS_DIR}/frameworks/ai/src/asr
        ${NUTTX_APPS_DIR}/frameworks/ai/include)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_ai_speech)
endif()

if(CONFIG_FEATURE_SYSTEM_VELACLAW)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/velaclaw_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jidl/velaclaw.jidl)
    target_include_directories(feature_impls PRIVATE
        ${NUTTX_APPS_DIR}/packages/ai_agent/include
        ${NUTTX_APPS_DIR}/packages/ai_agent/src/quickapp)

    # Bridge: mqueue-based IPC to AI agent process (pure POSIX, no external SDK)
    target_sources(feature_impls PRIVATE
        ${NUTTX_APPS_DIR}/packages/ai_agent/src/quickapp/velaclaw_quickapp_bridge.c)

    nuttx_add_jidl(
        TARGET
        feature_impls
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        system_velaclaw)
endif()

# for feature registry
target_sources(feature_impls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ajs_features_registry.cpp)
