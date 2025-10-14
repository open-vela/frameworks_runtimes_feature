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

TEST_FEATURE_OUT_PATH = $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/generated/jidl

ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/generated/jidl
endif

include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/array_null/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/event_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/jsonobject_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/const_object_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/interface_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/promise_callback/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/callback_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/simple_test/Makefile
include $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/permissions_test/Makefile

distclean::
	rm -rf $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/generated/jidl/*
