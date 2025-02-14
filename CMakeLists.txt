cmake_minimum_required(VERSION 3.22..3.29)

set(
  CMAKE_TOOLCHAIN_FILE
  "${CMAKE_SOURCE_DIR}/third-party/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file"
)

project(magic_args VERSION 0.2.0 LANGUAGES CXX)

if (MSVC)
  add_compile_options(
    # Include content and marker in error messages
    "/diagnostics:caret"
    # Source is UTF-8
    "/utf-8"
  )
  if (PROJECT_IS_TOP_LEVEL)
    add_compile_options(
      # C++ standard exception handling
      "/EHsc"
      # Enable most warnings
      "/W4"
      # Error on warning
      "/WX"
    )
    # Clang-CL
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      add_compile_options(
        "-Wno-missing-field-initializers"
        "-Wno-missing-designated-field-initializers"
        # As of 2025-02-13, the version of Clang-CL on GitHub actions does not recognize
        # -Wno-missing-designated-field-initializers
        "-Wno-unknown-warning-option"
      )
    endif ()
  endif ()
endif ()

add_subdirectory(magic_args)

option(BUILD_TESTING "Build tests" "${PROJECT_IS_TOP_LEVEL}")
if (BUILD_TESTING)
  include(CTest)
  add_subdirectory(tests)
endif ()

option(BUILD_EXAMPLES "Build examples" "${PROJECT_IS_TOP_LEVEL}")
if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()
