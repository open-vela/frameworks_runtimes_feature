if(CONFIG_FEATURE_TEST_FEATURES)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/jsonobject_test_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/jsonobject_test.jidl)

    nuttx_add_jidl(
        TARGET
        test_features
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        jsonobject_test)
endif()