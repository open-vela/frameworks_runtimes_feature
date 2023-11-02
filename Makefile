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
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_registry.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/promise_manager.cpp

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src

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

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/locale.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/locale_impl.cpp
FEATURELIST += locale

CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/error.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/error_impl.cpp
FEATURELIST += Error

ifeq ($(CONFIG_MESSAGE_CHANNEL),y)
AIDLSRCS += $(shell find ./modules/aidl -name *.aidl)
AIDLFLAGS = --lang=cpp -Imodules/aidl/ -hmodules/aidl/ -omodules/aidl/
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/modules/aidl
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_transport.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_channel.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/message_channel_impl.cpp
FEATURELIST += system_messageChannel
endif

ifeq ($(CONFIG_MIWEAR_APPS),y)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/miwear/apps/applications/proxyquickapp
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/miwear/apps/applications/proxyquickapp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/jumpapp.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/modules/jumpapp_impl.cpp
FEATURELIST += jumpApp
endif

ifeq ($(CONFIG_TESTING_CMOCKA),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/mockatest.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/mockatest_impl.cpp
FEATURELIST += mockatest
endif

endif

ifeq ($(CONFIG_QUICKAPP_FOLME_ANIMENGINE_ADAPTER),y)
FEATURELIST += folme
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
