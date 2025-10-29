ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/arraybuffer_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/arraybuffer_test.cpp
	EVENT_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/event_test/arraybuffer_test.jidl
	FEATURELIST += arraybuffer_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(EVENT_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header arraybuffer_test.h --source arraybuffer_test.cpp
endif
