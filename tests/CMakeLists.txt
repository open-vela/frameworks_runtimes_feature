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
    list(APPEND FEATURE_UNIT_TEST_SRCS ${FEATURE_TOP}/tests/unit/unit_test.cpp)
    list(APPEND FEATURE_UNIT_TEST_INCLUDE ${FEATURE_INC_DIRS})
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
        ${FEATURE_UNIT_TEST_INCLUDE})
endif()

list(
    APPEND
    FEATURE_INC_DIRS
    ${FEATURE_TEST_DIR}/unit/jidl)

if(CONFIG_FEATURE_UNIT_TEST)
if(CONFIG_FEATURE_TEST_JSFILE)
    list(APPEND FEATURE_SRCS ${FEATURE_TOP}/tests/unit/unit_jidl_util.cpp)
    set(FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_struct.cpp)
    set(JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/struct_test.jidl)

    nuttx_add_jidl(
        TARGET
        quickappfeatures
        FEATURE_SRCS
        ${FEATURE_SRCS}
        JIDLS
        ${JIDL_PATHS}
        FEATURE_NAMES
        struct_test)
endif()
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/jidl/test_features)