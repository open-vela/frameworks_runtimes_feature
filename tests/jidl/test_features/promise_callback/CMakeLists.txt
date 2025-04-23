if(CONFIG_FEATURE_TEST_FEATURES)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/promise_callback_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/promise_callback.jidl)

    nuttx_add_jidl(
        TARGET
        test_features
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        promise_callback)
endif()