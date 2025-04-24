ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/interface_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/interface_test.cpp
	INTERFACE_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/interface_test/interface_test.jidl
	FEATURELIST += interface_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(INTERFACE_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header interface_test.h --source interface_test.cpp
endif
