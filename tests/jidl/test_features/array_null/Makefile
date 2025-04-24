ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/array_null_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/array_null.cpp
	ARRAY_NULL_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/array_null/array_null.jidl
	FEATURELIST += array_null

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(ARRAY_NULL_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header array_null.h --source array_null.cpp
endif