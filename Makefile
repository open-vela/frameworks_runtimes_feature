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

BIN := $(APPDIR)/staging/libfeature.a

CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_context_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_context.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_framework.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_instance_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_instance.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_registry.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/promise_manager.cpp

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

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_test.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_test_impl.cpp
FEATURELIST += struct_test

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/feature_timers.cpp
FEATURELIST += timers

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/app_path.cpp

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/locale.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/locale_impl.cpp
FEATURELIST += locale

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/error.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/error_impl.cpp
FEATURELIST += Error

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/exchange.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/exchange_impl.cpp
FEATURELIST += exchange

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/storage.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/storage_impl.cpp
FEATURELIST += storage

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/sensor.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/sensor_imp.cpp
FEATURELIST += sensor

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/device.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/device_impl.cpp
FEATURELIST += device

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/configuration.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/configuration_impl.cpp
FEATURELIST += configuration

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_native.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/crypto_impl.cpp
FEATURELIST += system_crypto

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/cipher.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/cipher_impl.cpp
FEATURELIST += system_cipher

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/net_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/fetch.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/fetch_impl.cpp
FEATURELIST += fetch

ifeq ($(CONFIG_SYSTEM_ACTIVITY_SERVICE),y)
PROGNAME += feature_test_cli
PRIORITY += 100
STACKSIZE += 4096
MAINSRC += $(APPDIR)/frameworks/base/feature/modules/feature_test_cli.cpp

AIDLSRCS += $(shell find ./modules/aidl -name *.aidl)
AIDLFLAGS = --lang=cpp -Imodules/aidl/ -hmodules/aidl/ -omodules/aidl/
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/modules/aidl
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_transport.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_channel.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_channel_impl.cpp
FEATURELIST += system_messageChannel
endif

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/jumpapp.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/jumpapp_impl.cpp
ifeq ($(CONFIG_QUICKAPP_VAPP_XMS), y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src/framework
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src/jse
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/am/include/app
endif
FEATURELIST += jumpApp

ifeq ($(CONFIG_MIPLAY),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/miplay_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/miplay_1_0_impl.cpp
FEATURELIST += Miplay
endif

ifeq ($(CONFIG_SYSTEM_PACKAGE_SERVICE), y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/package.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/package_impl.cpp
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
MAINSRC += $(APPDIR)/frameworks/base/feature/tests/jidl/test_main.cpp
endif

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/system/libuv/ext/include
CXXSRCS += ${APPDIR}/frameworks/quickapp/src/jse/modules/system/jse_apppath.cpp
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src/jse/modules/system/
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/request.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/request_impl.cpp
FEATURELIST += request

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/file.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/file_impl.cpp
FEATURELIST += file

endif

ifeq ($(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER),y)
FEATURELIST += system_folme
endif

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))

context::


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

include $(APPDIR)/frameworks/base/feature/Module.mk
include $(APPDIR)/Application.mk
