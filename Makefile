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

CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_ffi.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/src/feature_framework.cpp

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/base/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/quickapp/src


ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/promise_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/promise_1_0_impl.cpp
FEATURELIST += Promise_1_0

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/record_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/record_1_0_impl.cpp
FEATURELIST += Record_1_0

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/simple_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/simple_1_0_impl.cpp
FEATURELIST += Simple_1_0

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_1_0.cpp
CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/struct_1_0_impl.cpp
FEATURELIST += Struct_1_0

CXXSRCS += $(APPDIR)/frameworks/base/feature/tests/jidl/feature_timers.cpp
FEATURELIST += timers

endif



PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))

context::


clean::
	rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h

distclean::
	rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	$(call DELFILE, $(PDATLIST))

clean_context::
	$(call DELFILE, $(PDATLIST))
endif

include $(APPDIR)/frameworks/base/feature/Module.mk
include $(APPDIR)/Application.mk
