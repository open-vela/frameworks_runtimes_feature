#
# Copyright (C) 2020 Xiaomi Corporation
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

include $(APPDIR)/Make.defs

CXXEXT     := .cpp
CXXFLAGS   += -std=c++17

# workaround for gcc-13 warning
GCC_VERSION := $(shell gcc -dumpversion)
ifeq ($(shell expr $(GCC_VERSION) \>= 13), 1)
  CFLAGS += --param=min-pagesize=0
  CXXFLAGS += --param=min-pagesize=0
endif

ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)

ifneq ($(CONFIG_FEATURE_LOG_LEVEL),)
CXXFLAGS += -DFEATURE_LOG_LEVEL=$(CONFIG_FEATURE_LOG_LEVEL)
endif

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/src

JIDL_PATH :=
OUT_PATH :=
TEST_PATH :=

TS2WASM_RUNTIMELIB_ROOT := $(APPDIR)/frameworks/runtimes/typescript/ts2wasm/runtime-library

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_common.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_context_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_context.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_ffi_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_ffi.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/value_translator_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_prototype.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_prototype_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_instance_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_instance.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_manager_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_registry.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/promise_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_object_ref.cpp

ifeq ($(CONFIG_FEATURE_USE_WAMR),y)
CXXFLAGS += -DWASM_ENABLE_GC=1
CXXFLAGS += -DWASM_ENABLE_STRINGREF=1
CFLAGS += -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0
CXXFLAGS += -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0
CXXFLAGS += -D__STDC_VERSION__=0

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/interpreter
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/common/gc
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/shared/utils
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/shared/platform/nuttx
CXXFLAGS += ${INCDIR_PREFIX}${TS2WASM_RUNTIMELIB_ROOT}/libdyntype
CXXFLAGS += ${INCDIR_PREFIX}${TS2WASM_RUNTIMELIB_ROOT}/utils

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_context_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_ffi_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_instance_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_prototype_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_manager_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_wamr_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/value_translator_wamr.cpp
endif

GTEST_DIR = $(APPDIR)/external/googletest/googletest/googletest
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src
CXXFLAGS += ${INCDIR_PREFIX}$(GTEST_DIR)/include

ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)

ifeq ($(CONFIG_ACCOUNT_FEATURE),y)
FEATURELIST += service_internal_account
endif

ifeq ($(CONFIG_MIJIA_CAMERA_CLIENT),y)
CFEATURELIST += system_internal_micamera
endif

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/app_path.cpp

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/locale_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/feature_locale.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += locale

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/error_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/error.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += Error

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/device_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/device.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_device

ifeq ($(CONFIG_QUICKAPP_TEST_FRAMEWORK),y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/internal_test_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/internal_test.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_internal_test
endif

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/net_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/fetch_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/fetch.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_fetch

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/prompt_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/prompt.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_prompt

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/upload_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/uploadtask.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_uploadtask

ifeq ($(CONFIG_FEATURE_TEST_CLIENT), y)
PROGNAME += feature_test_cli
PRIORITY += 100
STACKSIZE += 8192000
MAINSRC += $(APPDIR)/frameworks/runtimes/feature/modules/feature_test_cli.cpp
endif

ifeq ($(CONFIG_SYSTEM_ACTIVITY_SERVICE), y)
AIDLSRCS += $(shell find ./modules/aidl -name *.aidl)
AIDLFLAGS = --lang=cpp -Imodules/aidl/ -hmodules/aidl/ -omodules/aidl/
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/modules/aidl
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/message_transport.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/message_channel_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/message_channel.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src
FEATURELIST += system_messageChannel
endif

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/jumpapp_impl.cpp
ifeq ($(CONFIG_QUICKAPP_VAPP_XMS), y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/jse
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/am/include/app
endif
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/src/framework/dom-protobuf
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/jumpapp.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += jumpApp

ifeq ($(CONFIG_QUICKAPP), y)
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/router.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/router_impl.cpp
FEATURELIST += system_router

JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/system_app.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/system_app_impl.cpp
FEATURELIST += system_app
endif

ifeq ($(CONFIG_MEDIA_FEATURE),y)
CFEATURELIST += system_volume
CFEATURELIST += system_audio
CFEATURELIST += system_media_session
endif

ifeq ($(CONFIG_LIBUV_EXTENSION),y)
ifeq ($(CONFIG_UNQLITE),y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/unqlite/unqlite
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/storage.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/storage_impl.cpp
FEATURELIST += system_storage
endif

ifeq ($(CONFIG_KVDB),y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/exchange_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/exchange.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_exchange
endif

ifeq ($(CONFIG_CRYPTO_MBEDTLS),y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/crypto_native.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/crypto_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/crypto_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/crypto.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_crypto

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/cipher_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/cipher.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_cipher
endif

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/configuration_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/configuration.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_configuration
endif


ifeq ($(CONFIG_UORB), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/sensor_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/sensor.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_sensor
endif

ifeq ($(CONFIG_BOARDCTL), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/power_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/power.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_internal_power
endif

ifeq ($(CONFIG_VIBRATOR), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/vibrator_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/vibrator.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_vibrator
endif

ifeq ($(CONFIG_SYSTEM_PACKAGE_SERVICE), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/package_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/package.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_internal_package
endif

ifeq ($(CONFIG_SYSTEM_ACTIVITY_SERVICE), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/activity_feature_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/activity.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_internal_activity
endif

CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/array_null_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/array_null.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += array_null

CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/promise_callback.cpp
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/promise_callback_impl.cpp
FEATURELIST += promise_callback

CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event_test_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/event_test.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += event_test

ifeq ($(CONFIG_LIB_GOOGLETEST), y)
#CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/jidl/feat_test.cpp
#CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/jidl/feat_test_impl.cpp
#FEATURELIST += feat_test
#
#PROGNAME += feat_test
#PRIORITY += 100
#STACKSIZE += 4096
#CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/jidl/builtin/builtin_console.cpp
#MAINSRC += $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_main.cpp
endif

ifeq ($(CONFIG_FEATURE_UNIT_TEST), y)
PROGNAME += feature_unit_test
PRIORITY += 100
STACKSIZE += 1024
MAINSRC += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_test.cpp
endif

ifeq ($(CONFIG_UTILS_CURL), y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/request.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/request_impl.cpp
FEATURELIST += system_request
endif

JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/file.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/file_impl.cpp
FEATURELIST += system_file

endif

ifeq ($(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER),y)
FEATURELIST += system_folme
endif

ifeq ($(CONFIG_LIB_ZLIB),y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib/contrib/minizip
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib/contrib/minizip
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib
JIDL_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/zip.jidl
OUT_PATH += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/modules/zip_impl.cpp
FEATURELIST += system_zip
endif

ifeq ($(CONFIG_BLUETOOTH_FEATURE), y)
CFEATURELIST += system_bluetooth
CFEATURELIST += system_bluetooth_bt

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CFEATURELIST += system_bluetooth_bt_a2dpsink
endif
endif

ifneq ($(CONFIG_SYSTEM_BRIGHTNESS_SERVICE), )
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/brightness_impl.cpp
JIDL_PATH   += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/brightness.jidl
OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += system_brightness
endif

ifeq ($(CONFIG_APP_WECHAT),y)
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/wechat_impl.cpp
JIDL_PATH   += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/wechat.jidl
OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/src/
FEATURELIST += service_wechat
endif

ifeq ($(CONFIG_MIPLAY_QAPP),y)
JIDL_PATH += $(APPDIR)/frameworks/connectivity/miplay_lite/app/feature/jidl/miplay.jidl
OUT_PATH += $(APPDIR)/frameworks/connectivity/miplay_lite/app/feature/
FEATURELIST += service_miplay
endif

ifeq ($(CONFIG_UORB),y)
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event_impl.cpp
JIDL_PATH   += $(APPDIR)/frameworks/runtimes/feature/modules/jidl/event.jidl
OUT_PATH    += $(APPDIR)/frameworks/runtimes/feature/modules/src/
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event/topic.cpp
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event/event_context.cpp
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event/topics/user_topic.cpp
CXXSRCS     += $(APPDIR)/frameworks/runtimes/feature/modules/event/topics/battery_topic.cpp
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/topics/include/system/
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/topics/include/system/
FEATURELIST += system_event
endif

ifeq ($(CONFIG_MIWEAR_COMMON),y)
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/miwear/common/base/include/
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/miwear/common/base/include/
endif

ifneq ($(CONFIG_LYRA_NEW_FEATURE), )

ifneq ($(CONFIG_LYRA_NEW_FEATURE_TRANSFER), )
FEATURELIST += system_internal_hyperchannel
endif

ifneq ($(CONFIG_LYRA_NEW_FEATURE_MESSAGE_CENTER), )
FEATURELIST += system_internal_messagecenter
endif

ifneq ($(CONFIG_LYRA_NEW_FEATURE_NETWORKING), )
FEATURELIST += system_internal_networking
endif

endif

ifeq ($(CONFIG_SERVICE_AGENT_CLIENT),y)
FEATURE_IGNORE_JIDL = 'utils'
FEATURELIST += $(shell find $(APPDIR)/vendor/xiaomi/vela/service_agent/feature/jidl/ \
    -name '*.jidl' | grep -Ev $(FEATURE_IGNORE_JIDL) | xargs awk -F '^module|@' '{gsub(/\./, "_", $$2); print $$2}' | grep -v '^$$')
endif

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))

CXXSRCS += $(strip $(foreach i, $(shell seq 1 $(words $(JIDL_PATH))),\
	$(eval jidl_path=$(word $(i), $(JIDL_PATH)))\
	$(eval out_path=$(word $(i), $(OUT_PATH)))\
	$(eval file_name=$(strip $(basename $(notdir $(word $(i), $(JIDL_PATH))) .jidl)))\
	$(out_path)/$(file_name).cpp\
))
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/ajs_features_registry.cpp

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libfeature.a
include $(APPDIR)/frameworks/runtimes/feature/Module.mk
endif

EXPORT_FILES := include/feature_types.h include/feature_context.h include/feature_description.h \
                include/feature_exports.h include/feature_main_exports.h include/feature_log.h \
                include/ajs_features_init.h src/README.md registry/README.md

clean::
	rm -rf $(APPDIR)/frameworks/runtimes/feature/src/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/runtimes/feature/src/ajs_features_registry.cpp

distclean::
	rm -rf $(APPDIR)/frameworks/runtimes/feature/src/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/runtimes/feature/src/ajs_features_registry.cpp
	$(call DELFILE, $(PDATLIST))

clean_context::
	$(call DELFILE, $(PDATLIST))
endif

include $(APPDIR)/Application.mk
