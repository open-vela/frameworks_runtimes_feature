if(CONFIG_FEATURE_TEST_FEATURES)
    set(FEATURE_SRCS ${CMAKE_CURRENT_LIST_DIR}/simple_impl.cpp)
    set(JIDL_PATHS ${CMAKE_CURRENT_LIST_DIR}/simple.jidl)

    nuttx_add_jidl(
        TARGET
        test_features
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        simple)
endif()