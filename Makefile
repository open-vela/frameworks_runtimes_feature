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

TS2WASM_RUNTIMELIB_ROOT := $(APPDIR)/frameworks/runtimes/typescript/ts2wasm/runtime-library

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_context_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_ffi_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/value_translator_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_prototype_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_instance_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_manager_qjs.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/feature_qjs_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/qjs/array_buffer_qjs.cpp

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_common.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_context.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_main_exports.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_ffi.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_prototype.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_instance.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_registry.cpp
ifeq ($(CONFIG_FEATURE_ENABLE_TRACKER),y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_tracker.cpp
endif
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/promise_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_object_ref.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/feature_permission.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/permissions_manager.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/protobuf/proto_reflection.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/protobuf/proto_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/framework_log.cpp

ifeq ($(CONFIG_FEATURE_USE_WAMR),y)
CXXFLAGS += -DWASM_ENABLE_GC=1
CXXFLAGS += -DWASM_ENABLE_STRINGREF=1
CFLAGS += -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0
CXXFLAGS += -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0
CXXFLAGS += -D__STDC_VERSION__=0

TS2WASM_RUNTIMELIB_ROOT := $(APPDIR)/frameworks/runtimes/typescript/ts2wasm/runtime-library
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/interpreter
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/iwasm/common/gc
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/shared/utils
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/wamr/wamr/core/shared/platform/nuttx
CXXFLAGS += ${INCDIR_PREFIX}${TS2WASM_RUNTIMELIB_ROOT}/libdyntype
CXXFLAGS += ${INCDIR_PREFIX}${TS2WASM_RUNTIMELIB_ROOT}/utils

CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_context_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_ffi_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_instance_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_prototype_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_manager_wamr.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/feature_wamr_utils.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/src/backend/wamr/value_translator_wamr.cpp
endif

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl

ifeq ($(CONFIG_FEATURE_TEST_CLIENT), y)
	PROGNAME += feature_test_cli
	PRIORITY += 100
	STACKSIZE += 8192000
	MAINSRC += $(APPDIR)/frameworks/runtimes/feature/tests/feature_test_cli.cpp
endif

ifeq ($(CONFIG_FEATURE_UNIT_TEST), y)
	PROGNAME += feature_unit_test
	PRIORITY += 100
	STACKSIZE += 16384
	MAINSRC += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_test.cpp

ifeq ($(CONFIG_FEATURE_TEST_JSFILE), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_jidl_util.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_test_jidl_struct.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/struct_test.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_test_jidl_any.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/any_test.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/unit_test_jidl_function.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/function_test.cpp

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/struct_test.jidl --out-dir \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl --header struct_test.h --source struct_test.cpp
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/any_test.jidl --out-dir \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl --header any_test.h --source any_test.cpp
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/function_test.jidl --out-dir \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl --header function_test.h --source function_test.cpp
endif
endif

ifeq ($(CONFIG_FEATURE_TEST_JSFILE), y)
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/jidl/feat_test_impl.cpp
CXXSRCS += $(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/feat_test.cpp

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl/feat_test.jidl --out-dir \
		$(APPDIR)/frameworks/runtimes/feature/tests/unit/jidl --header feat_test.h --source feat_test.cpp
endif

include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/Makefile

GTEST_DIR = $(APPDIR)/external/googletest/googletest/googletest
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/src
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs

CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/quickapp/inspector/include

CXXFLAGS += ${INCDIR_PREFIX}$(GTEST_DIR)/include

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libfeature.a
endif

EXPORT_FILES := include/feature_types.h include/feature_context.h include/feature_description.h \
                include/feature_exports.h include/feature_main_exports.h include/feature_log.h \
                include/feature_permission.h include/ajs_features_init.h include/feature_trace.h src/README.md

endif

include $(APPDIR)/Application.mk
