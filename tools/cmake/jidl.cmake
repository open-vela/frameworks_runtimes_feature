include(CMakeParseArguments)

# generate jidl cpp wrapper files
function(jidl_codegen_files)
    # Define the supported set of keywords
    set(prefix ARG)
    set(noValues)
    set(singleValues JIDL_TOOL_PATH OUT_PATH LANG)
    set(multiValues JIDL_FILES_CPP JIDL_FILES_C)
    # Process the arguments passed in
    cmake_parse_arguments(
        PARSE_ARGV 0
        ${prefix}
        "${noValues}" "${singleValues}" "${multiValues}"
    )
    foreach(arg IN LISTS singleValues multiValues)
        if (${arg} STREQUAL "JIDL_TOOL_PATH")
            set(JIDL_TOOL_PATH ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "OUT_PATH")
            set(OUT_PATH ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "LANG")
            set(LANG ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "JIDL_FILES_CPP")
            set(JIDL_FILES_CPP ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "JIDL_FILES_C")
            set(JIDL_FILES_C ${${prefix}_${arg}})
        endif()
    endforeach()
    set(GENERATED_FILES "")
    message("~~~~JIDL_FILES_CPP: ${JIDL_FILES_CPP}")
    foreach(JIDL_FILE ${JIDL_FILES_CPP})
        get_filename_component(JIDL_FILE_NAME ${JIDL_FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${OUT_PATH}/${JIDL_FILE_NAME}.cpp ${OUT_PATH}/${JIDL_FILE_NAME}.h
            COMMAND python3 ${JIDL_TOOL_PATH}/jsongensource.py ${JIDL_FILE} -lang ${LANG} -out-dir ${OUT_PATH} -header ${JIDL_FILE_NAME}.h -source ${JIDL_FILE_NAME}.cpp
        )
        list(APPEND GENERATED_FILES ${OUT_PATH}/${JIDL_FILE_NAME}.cpp ${OUT_PATH}/${JIDL_FILE_NAME}.h)
    endforeach(JIDL_FILE ${JIDL_FILES_CPP})
    set(JIDL_GENERATED_CPP_FILES ${GENERATED_FILES} PARENT_SCOPE)

    # generate C files
    message("~~~~JIDL_FILES_C: ${JIDL_FILES_C}")
    set(GENERATED_FILES "")
    foreach(JIDL_FILE ${JIDL_FILES_C})
        get_filename_component(JIDL_FILE_NAME ${JIDL_FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${OUT_PATH}/${JIDL_FILE_NAME}.c ${OUT_PATH}/${JIDL_FILE_NAME}.h
            COMMAND python3 ${JIDL_TOOL_PATH}/jsongensource.py ${JIDL_FILE} -lang c++ -out-dir ${OUT_PATH} -header ${JIDL_FILE_NAME}.h -source ${JIDL_FILE_NAME}.c
        )
        list(APPEND GENERATED_FILES ${OUT_PATH}/${JIDL_FILE_NAME}.c ${OUT_PATH}/${JIDL_FILE_NAME}.h)
    endforeach(JIDL_FILE ${JIDL_FILES_C})
    set(JIDL_GENERATED_C_FILES ${GENERATED_FILES} PARENT_SCOPE)

endfunction(jidl_codegen_files)

# generate feature_registery.cpp
function(gen_feature_registery_cpp)
    # Define the supported set of keywords
    set(prefix ARG)
    set(noValues)
    set(singleValues "")
    set(multiValues FEATURE_LIST_CPP FEATURE_LIST_C)
    # Process the arguments passed in
    cmake_parse_arguments(
        PARSE_ARGV 0
        ${prefix}
        "${noValues}" "${singleValues}" "${multiValues}"
    )
    set(FEATURE_LIST_CPP)
    set(FEATURE_LIST_C)
    foreach(arg IN LISTS singleValues multiValues)
        if(${arg} STREQUAL "FEATURE_LIST_CPP")
            set(FEATURE_LIST_CPP ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "FEATURE_LIST_C")
            set(FEATURE_LIST_C ${${prefix}_${arg}})
        endif()
    endforeach()

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "// clang-format off\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "/* This file is auto-generated, DO NOT EDIT IT. */\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#include \"feature_exports.h\"\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#include \"feature_description.h\"\n\n")
    foreach(feature ${FEATURE_LIST_CPP})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "bool jse_${feature}_initFeature(FeatureRegistryHandle handle);\n")
    endforeach(feature ${FEATURE_LIST_CPP})
    list(LENGTH FEATURE_LIST_CPP table_cpp_count)

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#ifdef __cplusplus\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "extern \"C\" {\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#endif //__cplusplus\n")
    foreach(feature ${FEATURE_LIST_C})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    bool jse_${feature}_initFeature(FeatureRegistryHandle handle);\n")
    endforeach(feature ${FEATURE_LIST_C})
    list(LENGTH FEATURE_LIST_C table_c_count)
    math(EXPR table_total_count "${table_cpp_count} + ${table_c_count}")

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#ifdef __cplusplus\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "}\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#endif //__cplusplus\n\n")

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "FeatureRegistryTable g_ajs_features_registry_table = {\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    .count = ${table_total_count},\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    .data = {\n")
    foreach(feature ${FEATURE_LIST_CPP})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    jse_${feature}_initFeature,\n")
    endforeach(feature ${FEATURE_LIST_CPP})
    foreach(feature ${FEATURE_LIST_C})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    jse_${feature}_initFeature,\n")
    endforeach(feature ${FEATURE_LIST_C})
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    }\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "};\n")
	string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "FeatureRegistryTableHandle g_ajs_features_registry = &g_ajs_features_registry_table;\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "// clang-format on\n")
    configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
        ${CMAKE_BINARY_DIR}/ajs_features_registry.cpp
        @ONLY
    )
endfunction(gen_feature_registery_cpp)

# generate feature registery files
function(jidl_codegen_registry)
    # Define the supported set of keywords
    set(prefix ARG)
    set(noValues)
    set(singleValues "")
    set(multiValues FEATURE_LIST_CPP FEATURE_LIST_C)
    # Process the arguments passed in
    cmake_parse_arguments(
        PARSE_ARGV 0
        ${prefix}
        "${noValues}" "${singleValues}" "${multiValues}"
    )
    set(FEATURE_LIST_CPP)
    set(FEATURE_LIST_C)
    foreach(arg IN LISTS singleValues multiValues)
        if(${arg} STREQUAL "FEATURE_LIST_CPP")
            set(FEATURE_LIST_CPP ${${prefix}_${arg}})
        elseif(${arg} STREQUAL "FEATURE_LIST_C")
            set(FEATURE_LIST_C ${${prefix}_${arg}})
        endif()
    endforeach()
    gen_feature_registery_cpp(
        FEATURE_LIST_CPP ${FEATURE_LIST_CPP}
        FEATURE_LIST_C ${FEATURE_LIST_C}
    )
    set(JIDL_GENERATED_FEATURE_REGISTERY_FILES ${CMAKE_BINARY_DIR}/ajs_features_registry.cpp PARENT_SCOPE)
endfunction(jidl_codegen_registry)