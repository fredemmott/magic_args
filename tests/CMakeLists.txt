add_library(
  test-lib
  STATIC
  output.cpp output.hpp
)
if (WIN32)
  target_sources(test-lib PRIVATE output-windows.cpp)
else ()
  target_sources(test-lib PRIVATE output-posix.cpp)
endif ()

find_package(catch2 CONFIG REQUIRED)

add_executable(split-header-tests test.cpp)
target_link_libraries(split-header-tests PRIVATE test-lib Catch2::Catch2 Catch2::Catch2WithMain magic_args)

add_executable(single-header-tests test.cpp)
target_link_libraries(single-header-tests PRIVATE test-lib Catch2::Catch2 Catch2::Catch2WithMain magic_args-single-header)

include("${catch2_DIR}/Catch.cmake")
catch_discover_tests(split-header-tests single-header-tests)