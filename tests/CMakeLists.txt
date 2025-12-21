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

add_compile_definitions(
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

find_package(Catch2 CONFIG REQUIRED)
include("${Catch2_DIR}/Catch.cmake")

add_library(common-test-deps INTERFACE)
add_library(split-headers-test-deps INTERFACE)
add_library(single-header-test-deps INTERFACE)
target_link_libraries(common-test-deps INTERFACE test-lib Catch2::Catch2 Catch2::Catch2WithMain)
target_link_libraries(split-headers-test-deps INTERFACE common-test-deps magic_args)
target_link_libraries(single-header-test-deps INTERFACE common-test-deps magic_args-single-header)


option(ENABLE_SPLIT_HEADER_TESTS "Build and run tests in split-header-mode" ON)
if (ENABLE_SPLIT_HEADER_TESTS)
  message(STATUS "Split-header tests will be included in test runs")
  add_custom_target(all-split-header-tests)
else ()
  message(WARNING "Split-header tests are excluded from this build and test run")
endif ()
# This is off by default because:
# - split-headers provide a much better developer experience when debugging the tests, especially
#   when IDE ctest/catch2 support comes into the picture
# - similarly, IDE integrations work better when each `TEST_CASE()` is only in a single executable
#
# Depend on CI for this.
option(ENABLE_SINGLE_HEADER_TESTS "Build and run tests in single-header mode" OFF)
if (ENABLE_SINGLE_HEADER_TESTS)
  add_custom_target(all-single-header-tests)
  message(STATUS "Single-header tests will be included in test runs")
else ()
  message(WARNING "Single-header tests are excluded from this build and test run")
endif ()

function(add_test_executables NAME)
  set(SOURCES "${ARGN}")

  if (ENABLE_SPLIT_HEADER_TESTS)
    set(SPLIT_HEADERS_NAME "split-headers-${NAME}")

    add_executable("${SPLIT_HEADERS_NAME}" ${SOURCES})
    target_link_libraries("${SPLIT_HEADERS_NAME}" PRIVATE split-headers-test-deps)
    catch_discover_tests("${SPLIT_HEADERS_NAME}" PROPERTIES LABELS "split-headers" TEST_PREFIX "split-headers:${NAME}:")
    set_target_properties(
      "${SPLIT_HEADERS_NAME}"
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/split-headers"
      OUTPUT_NAME
      "${NAME}"
    )

    add_dependencies(all-split-header-tests "${SPLIT_HEADERS_NAME}")
  endif ()

  if (ENABLE_SINGLE_HEADER_TESTS)
    set(SINGLE_HEADER_NAME "single-header-${NAME}")

    add_executable("${SINGLE_HEADER_NAME}" ${SOURCES})
    target_link_libraries("${SINGLE_HEADER_NAME}" PRIVATE single-header-test-deps)
    target_compile_definitions("${SINGLE_HEADER_NAME}" PRIVATE TEST_SINGLE_HEADER)
    catch_discover_tests("${SINGLE_HEADER_NAME}" PROPERTIES LABELS "single-header" TEST_PREFIX "single-header:${NAME}:")

    set_target_properties(
      "${SINGLE_HEADER_NAME}"
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/single-header"
      OUTPUT_NAME
      "${NAME}"
    )

    add_dependencies(all-single-header-tests "${SINGLE_HEADER_NAME}")
  endif ()
endfunction()

# Core features
add_test_executables(test-core test-core.cpp)
add_test_executables(test-counted_flag test-counted_flag.cpp)
add_test_executables(test-default-values test-default-values.cpp)
add_test_executables(test-help test-help.cpp)
add_test_executables(test-std-optional test-std-optional.cpp)
add_test_executables(test-styles test-styles.cpp)
add_test_executables(test-constexpr_strings test-constexpr_strings.cpp)

# Extra features building on top of the core
add_test_executables(test-dump test-dump.cpp)
add_test_executables(test-encoding test-encoding.cpp)
add_test_executables(test-parse_subcommands_silent test-parse_subcommands_silent.cpp)
add_test_executables(test-parse_subcommands test-parse_subcommands.cpp)
add_test_executables(test-invoke_subcommands_silent test-invoke_subcommands_silent.cpp)
add_test_executables(test-invoke_subcommands test-invoke_subcommands.cpp)
add_test_executables(test-multicall test-multicall.cpp)

if (APPLE)
  find_package(Iconv REQUIRED)
  target_link_libraries(split-headers-test-encoding PRIVATE Iconv::Iconv)
  target_link_libraries(single-header-test-encoding PRIVATE Iconv::Iconv)
endif ()

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
