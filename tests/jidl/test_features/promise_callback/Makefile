ifeq ($(CONFIG_FEATURE_TEST_FEATURES),y)
	CXXSRCS     := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/promise_callback_impl.cpp
	CXXSRCS	    := $(CXXSRCS) $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/../generated/jidl/promise_callback.cpp
	PROMISE_CALLBACK_JIDL_PATH 	= $(APPDIR)/frameworks/runtimes/feature/tests/jidl/test_features/promise_callback/promise_callback.jidl
	FEATURELIST += promise_callback

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
	$(PROMISE_CALLBACK_JIDL_PATH) --out-dir $(TEST_FEATURE_OUT_PATH) \
	--header promise_callback.h --source promise_callback.cpp
endif