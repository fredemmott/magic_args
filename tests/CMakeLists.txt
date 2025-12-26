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

if (ENABLE_SINGLE_HEADER)
  add_compile_definitions(TEST_SINGLE_HEADER)
endif ()

find_package(Catch2 CONFIG REQUIRED)
include("${Catch2_DIR}/Catch.cmake")

function(add_test_executables TARGET)
  set(SOURCES "${ARGN}")

  add_executable("${TARGET}" ${SOURCES})
  target_link_libraries("${TARGET}" PRIVATE test-lib Catch2::Catch2WithMain magic_args)
  catch_discover_tests("${TARGET}" PROPERTIES TEST_PREFIX "${TARGET}:")
endfunction()

# Core features
add_test_executables(test-core test-core.cpp)
add_test_executables(test-counted_flag test-counted_flag.cpp)
add_test_executables(test-default-values test-default-values.cpp)
add_test_executables(test-demangling test-demangling.cpp)
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
add_test_executables(test-subcommand_names test-subcommand_names.cpp)
add_test_executables(test-multicall test-multicall.cpp)

if (APPLE)
  find_package(Iconv REQUIRED)
  target_link_libraries(test-encoding PRIVATE Iconv::Iconv)
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

if (NOT (BUILD_EXAMPLES AND BUILD_UTILITIES))
  message(WARNING "Skipping inspect-subcommands tests")
else ()
  message(STATUS "Enabling inspect-subcommands tests")
  include(test-inspect-subcommands.cmake)
endif ()