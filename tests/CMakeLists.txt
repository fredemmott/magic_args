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

target_link_libraries(
  test-lib
  PUBLIC
  magic_args
)

find_package(catch2 CONFIG REQUIRED)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE test-lib Catch2::Catch2 Catch2::Catch2WithMain)

include(CTest)
include("${catch2_DIR}/Catch.cmake")
catch_discover_tests(test)