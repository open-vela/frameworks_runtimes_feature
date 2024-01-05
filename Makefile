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
ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)

ifneq ($(CONFIG_FEATURE_LOG_LEVEL),)
CXXFLAGS += -DFEATURE_LOG_LEVEL=$(CONFIG_FEATURE_LOG_LEVEL)
endif

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/modules/src

JIDL_PATH :=
OUT_PATH :=
TEST_PATH :=

CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_common.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_context_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_context.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/value_translator_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_prototype.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_prototype_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_instance_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_instance.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_registry.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/promise_manager.cpp

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
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/runtime-library/libdyntype
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/runtime-library/utils

CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_context_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_instance_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_prototype_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_wamr_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/value_translator_wamr.cpp
endif

GTEST_DIR = $(APPDIR)/external/googletest/googletest/googletest
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src
CXXFLAGS += ${INCDIR_PREFIX}$(GTEST_DIR)/include

ifeq ($(CONFIG_ARCH), arm)
TARGETDIR := arm
else ifeq ($(CONFIG_ARCH), arm64)
TARGETDIR := aarch64
else ifeq ($(CONFIG_ARCH), xtensa)
TARGETDIR := xtensa
else
TARGETDIR := x86
endif
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/libffi
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/libffi
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/libffi/libffi/src/$(TARGETDIR)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/libffi/libffi/src/$(TARGETDIR)

ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/promise_test.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/promise_test_impl.cpp
FEATURELIST += promise_test

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/record_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/record_1_0_impl.cpp
FEATURELIST += Record

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/simple_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/simple_1_0_impl.cpp
FEATURELIST += Simple

CSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_test.c
CSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_test_impl.c
CFEATURELIST += struct_test

ifeq ($(CONFIG_ACCOUNT_FEATURE),y)
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/account/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/account/include
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/account_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/account/feature/account_1_0_impl.cpp
FEATURELIST += service_internal_account
endif

ifeq ($(CONFIG_MIJIA_CAMERA_CLIENT),y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/mijia_camera_client/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/mijia_camera_client/feature
CXXSRCS += $(APPDIR)/frameworks/mijia_camera_client/feature/micamera.cpp
CXXSRCS += $(APPDIR)/frameworks/mijia_camera_client/feature/micamera_impl.cpp
FEATURELIST += system_internal_micamera
endif

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/app_path.cpp

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/locale_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/feature_locale.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += locale

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/error_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/error.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += Error

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/device_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/device.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_device

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/net_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/fetch_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/fetch.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_fetch

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/prompt_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/prompt.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_prompt

ifeq ($(CONFIG_SYSTEM_ACTIVITY_SERVICE),y)
PROGNAME += feature_test_cli
PRIORITY += 100
STACKSIZE += 8192000
MAINSRC += $(APPDIR)/frameworks/base/feature/modules/feature_test_cli.cpp

AIDLSRCS += $(shell find ./modules/aidl -name *.aidl)
AIDLFLAGS = --lang=cpp -Imodules/aidl/ -hmodules/aidl/ -omodules/aidl/
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/modules/aidl
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_transport.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_channel_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/message_channel.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src
FEATURELIST += system_messageChannel
endif

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/jumpapp_impl.cpp
ifeq ($(CONFIG_QUICKAPP_VAPP_XMS), y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src/framework
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src/jse
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/am/include/app
endif
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/jumpapp.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += jumpApp

ifeq ($(CONFIG_MEDIA_FEATURE),y)
FEATURELIST += system_volume
CFEATURELIST += system_audio
endif

ifeq ($(CONFIG_LIBUV_EXTENSION),y)
ifeq ($(CONFIG_UNQLITE),y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/unqlite/unqlite
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/storage.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/storage_impl.cpp
FEATURELIST += system_storage
endif

ifeq ($(CONFIG_KVDB),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/exchange_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/exchange.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_exchange
endif

ifeq ($(CONFIG_CRYPTO_MBEDTLS),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_native.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/crypto.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_crypto

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/cipher_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/cipher.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_cipher
endif

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/configuration_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/configuration.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_configuration
endif


ifeq ($(CONFIG_UORB), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/sensor_imp.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/sensor.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += sensor
endif

ifeq ($(CONFIG_BOARDCTL), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/power_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/power.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_internal_power
endif

ifeq ($(CONFIG_SYSTEM_PACKAGE_SERVICE), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/package_impl.cpp
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/package.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
FEATURELIST += system_internal_package
endif

ifeq ($(CONFIG_SYSTEM_ACTIVITY_SERVICE), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/activity_feature.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/activity_feature_impl.cpp
FEATURELIST += system_internal_activity
endif

ifeq ($(CONFIG_LIB_GOOGLETEST), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/feat_test.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/feat_test_impl.cpp
FEATURELIST += feat_test

PROGNAME += feat_test
PRIORITY += 100
STACKSIZE += 4096
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/builtin/builtin_console.cpp
MAINSRC += $(APPDIR)/frameworks/base/feature/tests/jidl/test_main.cpp
endif

ifeq ($(CONFIG_UTILS_CURL), y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/request.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/request_impl.cpp
FEATURELIST += system_request
endif

JIDL_PATH += $(APPDIR)/frameworks/base/feature/modules/jidl/file.jidl
OUT_PATH += $(APPDIR)/frameworks/base/feature/modules/src/
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/file_impl.cpp
FEATURELIST += system_file

endif

ifeq ($(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/folme.cpp
FEATURELIST += system_folme
endif

ifeq ($(CONFIG_BLUETOOTH_FEATURE), y)
CFEATURELIST += system_bluetooth
CFEATURELIST += system_bluetooth_bt

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CFEATURELIST += system_bluetooth_bt_a2dpsink
endif
endif

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))

CXXSRCS += $(strip $(foreach i, $(shell seq 1 $(words $(JIDL_PATH))),\
	$(eval jidl_path=$(word $(i), $(JIDL_PATH)))\
	$(eval out_path=$(word $(i), $(OUT_PATH)))\
	$(eval file_name=$(strip $(basename $(notdir $(word $(i), $(JIDL_PATH))) .jidl)))\
	$(out_path)/$(file_name).cpp\
))

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libfeature.a
include $(APPDIR)/frameworks/base/feature/Module.mk
endif

EXPORT_FILES := include/feature_types.h include/feature_context.h include/feature_description.h \
                include/feature_exports.h include/feature_main_exports.h include/feature_log.h \
                src/ajs_features_init.h src/README.md registry/README.md

clean::
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h

distclean::
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	$(call DELFILE, $(PDATLIST))

clean_context::
	$(call DELFILE, $(PDATLIST))
endif

include $(APPDIR)/Application.mk
