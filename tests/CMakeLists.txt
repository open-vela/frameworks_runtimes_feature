if(CONFIG_FEATURE_TEST_CLIENT)
    list(APPEND FEATURE_CLI_SRCS ${FEATURE_TEST_DIR}/feature_test_cli.cpp)
    list(APPEND FEATURE_CLI_INC_DIRS ${FEATURE_TOP}/include)
    nuttx_add_application(
        NAME
        feature_test_cli
        PRIORITY
        100
        STACKSIZE
        4096000
        DEPENDS
        libfeature
        SRCS
        ${FEATURE_CLI_SRCS}
        INCLUDE_DIRECTORIES
        ${FEATURE_CLI_INC_DIRS})
endif()

if(CONFIG_FEATURE_UNIT_TEST)
    list(APPEND FEATURE_UNIT_TEST_SRCS ${FEATURE_TEST_DIR}/unit/unit_test.cpp)
    list(APPEND FEATURE_UNIT_TEST_SRCS ${FEATURE_INC_DIRS})
    nuttx_add_application(
        NAME
        feature_unit_test
        PRIORITY
        100
        STACKSIZE
        16384
        DEPENDS
        libfeature
        SRCS
        ${FEATURE_UNIT_TEST_SRCS}
        INCLUDE_DIRECTORIES
        ${FEATURE_UNIT_TEST_SRCS})
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/jidl/test_features)