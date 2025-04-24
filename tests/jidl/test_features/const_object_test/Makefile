ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/const_object_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/const_object_test.cpp
	CONST_OBJECT_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/const_object_test/const_object_test.jidl
	FEATURELIST += const_object_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(CONST_OBJECT_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header const_object_test.h --source const_object_test.cpp
endif
