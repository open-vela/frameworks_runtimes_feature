ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/simple_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/simple_test.cpp
	SIMPLE_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/simple_test/simple_test.jidl
	FEATURELIST += simple_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(SIMPLE_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header simple_test.h --source simple_test.cpp
endif
