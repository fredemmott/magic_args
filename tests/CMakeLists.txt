add_library(
  test-lib
  STATIC
  output.cpp output.hpp chomp.hpp
)
target_compile_features(
  test-lib
  PUBLIC
  cxx_std_23
)
if (WIN32)
  target_sources(test-lib PRIVATE output-windows.cpp)
else ()
  target_sources(test-lib PRIVATE output-posix.cpp)
endif ()

find_package(Catch2 CONFIG REQUIRED)
include("${Catch2_DIR}/Catch.cmake")

add_library(common-test-deps INTERFACE)
add_library(split-headers-test-deps INTERFACE)
add_library(single-header-test-deps INTERFACE)
target_link_libraries(common-test-deps INTERFACE test-lib Catch2::Catch2 Catch2::Catch2WithMain)
target_link_libraries(split-headers-test-deps INTERFACE common-test-deps magic_args)
target_link_libraries(single-header-test-deps INTERFACE common-test-deps magic_args-single-header)

add_custom_target(all-split-header-tests)
add_custom_target(all-single-header-tests)

function(add_test_executables NAME)
  set(SOURCES "${ARGN}")

  set(SPLIT_HEADERS_NAME "split-headers-${NAME}")
  set(SINGLE_HEADER_NAME "single-header-${NAME}")

  add_executable("${SPLIT_HEADERS_NAME}" ${SOURCES})
  target_link_libraries("${SPLIT_HEADERS_NAME}" PRIVATE split-headers-test-deps)
  catch_discover_tests("${SPLIT_HEADERS_NAME}" PROPERTIES LABELS "split-headers" TEST_PREFIX "split-headers:")

  add_executable("${SINGLE_HEADER_NAME}" ${SOURCES})
  target_link_libraries("${SINGLE_HEADER_NAME}" PRIVATE single-header-test-deps)
  target_compile_definitions("${SINGLE_HEADER_NAME}" PRIVATE TEST_SINGLE_HEADER)
  catch_discover_tests("${SINGLE_HEADER_NAME}" PROPERTIES LABELS "single-header" TEST_PREFIX "single-header:")

  add_dependencies(all-split-header-tests "${SPLIT_HEADERS_NAME}")
  add_dependencies(all-single-header-tests "${SINGLE_HEADER_NAME}")
endfunction()

add_test_executables(test test.cpp)
add_test_executables(test-styles test-styles.cpp)
add_test_executables(test-parse_subcommands-silent test-parse_subcommands_silent.cpp)

if (WIN32)
  add_test_executables(test-windows test-windows.cpp utf8-process-code-page.manifest)
endif ()

get_target_property(HAVE_ENUM_SUPPORT magic_args MAGIC_ARGS_HAVE_ENUM_SUPPORT)
if (HAVE_ENUM_SUPPORT)
  message(STATUS "Enabling enum tests")
  add_test_executables(test-enums test-enums.cpp)
else ()
  message(WARNING "Enums are not supported, not testing enums")
endif ()
