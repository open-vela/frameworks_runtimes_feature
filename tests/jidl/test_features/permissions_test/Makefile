ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/permissions_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/permissions_test.cpp
	PERMISSIONS_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/permissions_test/permissions_test.jidl
	FEATURELIST += permissions_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(PERMISSIONS_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header permissions_test.h --source permissions_test.cpp
endif