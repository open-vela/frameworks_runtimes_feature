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
        131072
        DEPENDS
        libfeature
        SRCS
        ${FEATURE_UNIT_TEST_SRCS}
        INCLUDE_DIRECTORIES
        ${FEATURE_UNIT_TEST_INCLUDE})
endif()

if(CONFIG_FEATURE_TEST_JSFILE)
    list(APPEND FEATURE_INC_DIRS ${FEATURE_TEST_DIR}/unit/jidl)
    nuttx_add_library(feature_test_jsfile STATIC)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/jidl/feat_test_impl.cpp)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/feat_test.jidl)
    list(APPEND TEST_FEATURE_NAMES feat_test)

if(CONFIG_FEATURE_UNIT_TEST)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_jidl_util.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_struct.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_any.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_function.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_const.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_interface.cpp)
    list(APPEND FEAT_TEST_FEATURE_SRCS ${FEATURE_TEST_DIR}/unit/unit_test_jidl_jsonobject.cpp)

    # set featuree jidl path
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/struct_test.jidl)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/any_test.jidl)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/function_test.jidl)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/unit_const_test.jidl)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/unit_interface_test.jidl)
    list(APPEND FEAT_TEST_JIDL_PATHS ${FEATURE_TEST_DIR}/unit/jidl/unit_jsonobject_test.jidl)
    # set feature name
    list(APPEND TEST_FEATURE_NAMES struct_test any_test function_test unit_const_test unit_interface_test)
    list(APPEND TEST_FEATURE_NAMES unit_jsonobject_test)
endif()

    nuttx_add_jidl(
        TARGET
        feature_test_jsfile
        FEATURE_SRCS
        ${FEAT_TEST_FEATURE_SRCS}
        JIDLS
        ${FEAT_TEST_JIDL_PATHS}
        FEATURE_NAMES
        ${TEST_FEATURE_NAMES})
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/jidl/test_features)