include(CMakeParseArguments)

# generate jidl cpp wrapper files
function(jidl_codegen_files)
    # Define the supported set of keywords
    set(prefix ARG)
    set(noValues)
    set(singleValues JIDL_TOOL_PATH OUT_PATH)
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
            COMMAND python3 ${JIDL_TOOL_PATH}/jsongensource.py ${JIDL_FILE} -out-dir ${OUT_PATH} -header ${JIDL_FILE_NAME}.h -source ${JIDL_FILE_NAME}.cpp
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
            COMMAND python3 ${JIDL_TOOL_PATH}/jsongensource.py ${JIDL_FILE} -out-dir ${OUT_PATH} -header ${JIDL_FILE_NAME}.h -source ${JIDL_FILE_NAME}.c
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
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "/* This file is auto-generated, DO NOT EDIT IT. */\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#include \"ajs_features_registry.h\"\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#include \"ajs_features_list.h\"\n\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "bool registerAjsFeatures(FeatureRegistryHandle handle) {\n")
    foreach(feature ${FEATURE_LIST_CPP})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    jse_${feature}_initFeature(handle);\n")
    endforeach(feature ${FEATURE_LIST_CPP})
    foreach(feature ${FEATURE_LIST_C})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    jse_${feature}_initFeature(handle);\n")
    endforeach(feature ${FEATURE_LIST_C})
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    return true;\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "}\n")
    configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
        ${CMAKE_BINARY_DIR}/ajs_features_registry.cpp
        @ONLY
    )
endfunction(gen_feature_registery_cpp)

# generate ajs_features_list.h
function(gen_feature_registery_h)

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

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "/* This file is auto-generated, DO NOT EDIT IT. */\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#ifndef AJS_FEATURES_LIST_H_\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#define AJS_FEATURES_LIST_H_\n\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#include \"feature_exports.h\"\n\n")
    foreach(feature ${FEATURE_LIST_CPP})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "bool jse_${feature}_initFeature(FeatureRegistryHandle handle);\n")
    endforeach(feature ${FEATURE_LIST_CPP})

    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#ifdef __cplusplus\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "extern \"C\" {\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#endif //__cplusplus\n")
    foreach(feature ${FEATURE_LIST_C})
        string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "    bool jse_${feature}_initFeature(FeatureRegistryHandle handle);\n")
    endforeach(feature ${FEATURE_LIST_C})
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#ifdef __cplusplus\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "}\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "#endif //__cplusplus\n")
    string(APPEND CMAKE_CONFIGURABLE_FILE_CONTENT "\n#endif")
    configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
        ${CMAKE_BINARY_DIR}/ajs_features_list.h
        @ONLY
    )
endfunction(gen_feature_registery_h)

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
    gen_feature_registery_h(
        FEATURE_LIST_CPP ${FEATURE_LIST_CPP}
        FEATURE_LIST_C ${FEATURE_LIST_C}
    )
    set(JIDL_GENERATED_FEATURE_REGISTERY_FILES ${CMAKE_BINARY_DIR}/ajs_features_registry.cpp ${CMAKE_BINARY_DIR}/ajs_features_list.h PARENT_SCOPE)
endfunction(jidl_codegen_registry)