# Helper function to create a test executable.
#
# Usage:
#   add_gtest(
#       NAME my_test
#       SOURCES test1.cpp test2.cpp
#       LIBS mylib otherlib
#   )
#
# Tests link src/testing/gtest_main.cc instead of GTest::gtest_main so that
# death tests do not write a core dump per run.
function(add_gtest)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;LIBS" ${ARGN})
    if(NOT TARGET cnc_gtest_main)
        add_library(cnc_gtest_main STATIC "${CMAKE_SOURCE_DIR}/src/testing/gtest_main.cc")
        target_link_libraries(cnc_gtest_main PUBLIC GTest::gtest port)
    endif()
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE cnc_gtest_main GTest::gmock ${ARG_LIBS})
    gtest_discover_tests(${ARG_NAME})
endfunction()
