ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/jsonobject_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/jsonobject_test.cpp
	JSONOBJECT_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/jsonobject_test/jsonobject_test.jidl
	FEATURELIST += jsonobject_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(JSONOBJECT_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header jsonobject_test.h --source jsonobject_test.cpp
endif