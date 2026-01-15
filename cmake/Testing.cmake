# Helper function to create a test executable.
#
# Usage:
#   add_gtest(
#       NAME my_test
#       SOURCES test1.cpp test2.cpp
#       LIBS mylib otherlib
#   )
function(add_gtest)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;LIBS" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE GTest::gtest_main GTest::gmock ${ARG_LIBS})
    gtest_discover_tests(${ARG_NAME})
endfunction()
