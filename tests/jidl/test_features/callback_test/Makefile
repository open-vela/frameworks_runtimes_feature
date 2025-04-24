ifeq ($(CONFIG_FEATURE_TEST_FEATURES), y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/callback_test_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/callback_test.cpp
	CALLBACL_TEST_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/callback_test/callback_test.jidl
	FEATURELIST += callback_test

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(CALLBACL_TEST_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header callback_test.h --source callback_test.cpp
endif
