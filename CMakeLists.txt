
# find libffi and rapidjson library
find_package(libffi CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)


# declare source directory variable
set(QUICKAPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../quickapp/src)
set(FEATURE_FRAMEWORK_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(FEATURE_FRAMEWORK_INC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# set runtime engine
set(FEATURE_BACKEND_ENGINE "quickjs" CACHE STRING "feature backend engine, support quickjs and ts2wasm, default: quickjs")
set_property(CACHE FEATURE_BACKEND_ENGINE PROPERTY STRINGS "quickjs;ts2wasm")
if(FEATURE_BACKEND_ENGINE STREQUAL "quickjs")
    # setup quickjs
    set(QUICKJS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../apps/interpreters/quickjs/)
    file(STRINGS "${QUICKJS_DIR}/quickjs/VERSION" QJS_VERSION_STR)

    add_library(backend_engine STATIC
        ${QUICKJS_DIR}/quickjs/cutils.c
        ${QUICKJS_DIR}/quickjs/libbf.c
        ${QUICKJS_DIR}/quickjs/libregexp.c
        ${QUICKJS_DIR}/quickjs/libunicode.c
        ${QUICKJS_DIR}/quickjs/quickjs-debugger.c
        ${QUICKJS_DIR}/quickjs/quickjs.c
    )
    set_target_properties(backend_engine PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
    # setup quickjs include directory
    target_include_directories(backend_engine PUBLIC
        ${QUICKJS_DIR}
    )
    # target_compile_options(backend_engine PRIVATE ${backend_engine_cflags})
    target_compile_definitions(backend_engine PUBLIC
        QJS_VERSION_STR="${QJS_VERSION_STR}"
        BACKEND_ENGINE_TYPE=0
    )
    if(CONFIG_INTERPRETERS_QUICKJS_DEBUG)
    target_compile_definitions(backend_engine PRIVATE
        CONFIG_INTERPRETERS_QUICKJS_DEBUG
    )
    endif(CONFIG_INTERPRETERS_QUICKJS_DEBUG)
    target_compile_definitions(backend_engine PRIVATE
        CONFIG_BIGNUM
        CONFIG_VERSION="${backend_engine_VERSION_STR}"
    )
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_definitions(backend_engine PRIVATE
            DUMP_LEAKS
        )
    endif()

    target_include_directories(backend_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_link_libraries(backend_engine atomic)
    endif()

    if(AIOTJS_ENABLE_SIM_DEBUG)
        target_compile_definitions(backend_engine PUBLIC AIOTJS_ENABLE_SIM_DEBUG)
    endif(AIOTJS_ENABLE_SIM_DEBUG)

    target_include_directories(backend_engine INTERFACE
        ${QUICKJS_DIR}
    )

    # set runtime source library
    #set(RUNTIME_SOURCE_FILES ${QUICKAPP_SRC_DIR}/jse/quickjs/jse_quickjs.cpp CACHE INTERNAL "runtime source files")

elseif(FEATURE_BACKEND_ENGINE STREQUAL "ts2wasm")

    set(RUNTIME_SOURCE_FILES "" CACHE INTERNAL "runtime source files")
    # BACKEND_ENGINE_TYPE=1
else()
    message(FATAL_ERROR "unsupported backend !")
endif()

file(GLOB FRAMEWORK_SOURCES CONFIG_DEPENDS ${FEATURE_FRAMEWORK_SRC}/*.cpp ${FEATURE_FRAMEWORK_SRC}/*.h)
file(GLOB FRAMEWORK_INC_HEADERS CONFIG_DEPENDS ${FEATURE_FRAMEWORK_INC}/*.h)

add_library(feature-framework STATIC
    ${FRAMEWORK_INC_HEADERS}
    ${FRAMEWORK_SOURCES}
)

target_include_directories(feature-framework PUBLIC
    ${FEATURE_FRAMEWORK_SRC}
    ${FEATURE_FRAMEWORK_INC}
)

# link backend engine
target_link_libraries(feature-framework PUBLIC
    backend_engine
    rapidjson
    libffi
)
