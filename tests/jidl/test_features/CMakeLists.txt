if(CONFIG_FEATURE_TEST_FEATURES)
  nuttx_add_library(test_features STATIC)

  file(GLOB_RECURSE FEATURE_TEST_CMAKE_LISTS "${CMAKE_CURRENT_LIST_DIR}/*/CMakeLists.txt")
  foreach(FEATURE_TEST_CMAKE_LIST ${FEATURE_TEST_CMAKE_LISTS})
    get_filename_component(FEATURE_TEST_SUBDIR ${FEATURE_TEST_CMAKE_LIST} DIRECTORY)
    add_subdirectory(${FEATURE_TEST_SUBDIR})
  endforeach()
endif()