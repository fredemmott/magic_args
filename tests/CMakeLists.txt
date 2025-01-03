add_library(
  test-lib
  STATIC
  output.hpp
  output-windows.cpp output-windows.hpp
)
target_link_libraries(
  test-lib
  PUBLIC
  magic_args
)

find_package(catch2 CONFIG REQUIRED)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE test-lib Catch2::Catch2 Catch2::Catch2WithMain)