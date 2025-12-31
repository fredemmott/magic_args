if (APPLE)
  find_package(Iconv REQUIRED)
endif ()

if (BUILD_UTILITIES)
  # Needs the 'enumerate-subcommands' utility
  include(MagicArgs)
endif ()

function(add_example PRIMARY_SOURCE)
  cmake_path(GET PRIMARY_SOURCE STEM EXAMPLE_NAME)
  set(TARGET "example-${EXAMPLE_NAME}")

  set(options SUBCOMMANDS)
  set(oneValueArgs "")
  set(multiValueArgs LINK_LIBRARIES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_executable("${TARGET}" ${ARG_UNPARSED_ARGUMENTS} "${PRIMARY_SOURCE}")
  target_link_libraries("${TARGET}" PRIVATE magic_args "${ARG_LINK_LIBRARIES}")

  set_target_properties("${TARGET}" PROPERTIES OUTPUT_NAME "${EXAMPLE_NAME}")

  if (ARG_SUBCOMMANDS AND BUILD_UTILITIES)
    # Links are only actually usable for multicall executables, but build them
    # for all subcommands to check that subcommand introspection works
    magic_args_enumerate_subcommands(
      "${TARGET}"
      HARDLINKS_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${TARGET}>-links"
    )
  endif ()

  if (APPLE)
    target_link_libraries("${TARGET}" PRIVATE Iconv::Iconv)
  endif ()
endfunction()

add_example(minimal.cpp)
add_example(minimal-powershell-style.cpp)
add_example(everything.cpp)
add_example(subcommands.cpp)
add_example(multicall.cpp SUBCOMMANDS)
add_example(multicall-powershell-style.cpp SUBCOMMANDS)

if (WIN32)
  add_library(win32-utf8-messagebox OBJECT win32_utf8_messagebox.cpp win32_utf8_messagebox.hpp)
  target_compile_features(win32-utf8-messagebox PUBLIC cxx_std_23)
  add_example(
    winmain.cpp
    WIN32
    LINK_LIBRARIES
    win32-utf8-messagebox
  )
  add_example(
    winmain-subcommands.cpp
    WIN32 SUBCOMMANDS
    LINK_LIBRARIES
    win32-utf8-messagebox
  )
  add_example(
    winmain-multicall.cpp
    WIN32 SUBCOMMANDS
    LINK_LIBRARIES
    win32-utf8-messagebox
  )
endif ()

if (BUILD_UTILITIES)
  option(INSTALL_MULTICALL_EXAMPLE "Install the multi-call example and symlinks" OFF)
  if (INSTALL_MULTICALL_EXAMPLE)
    install(TARGETS example-multicall)
    magic_args_install_multicall_links(
      example-multicall
      RELATIVE_SYMLINKS
      SYMLINKS_DESTINATION "bin"
    )
  endif ()
endif ()
