cmake_minimum_required(VERSION 3.22..3.29)

set(
  CMAKE_TOOLCHAIN_FILE
  "${CMAKE_SOURCE_DIR}/third-party/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file"
)

project(magic_args VERSION 0.0.1 LANGUAGES CXX)

if (MSVC)
  add_compile_options(
    # Include content and marker in error messages
    "/diagnostics:caret"
    # Source is UTF-8
    "/utf-8"
  )
endif ()

add_subdirectory(magic_args)

include(CTest)
if (BUILD_TESTING)
  add_subdirectory(tests)
endif ()

option(BUILD_EXAMPLES "Build examples" "${PROJECT_IS_TOP_LEVEL}")
if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()