ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/event_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/event_test.cpp
	EVENT_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/event_test/event_test.jidl
	FEATURELIST += event_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(EVENT_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header event_test.h --source event_test.cpp
endif