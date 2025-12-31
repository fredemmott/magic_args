if (APPLE)
  find_package(Iconv REQUIRED)
endif ()

set(SUBCOMMAND_EXAMPLES)

function(add_example TARGET)
  set(options SUBCOMMANDS)
  set(oneValueArgs "")
  set(multiValueArgs LIBRARIES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (ARG_SUBCOMMANDS)
    list(APPEND SUBCOMMAND_EXAMPLES "${TARGET}")
  endif ()

  add_executable("${TARGET}" ${ARG_UNPARSED_ARGUMENTS})
  target_link_libraries("${TARGET}" PRIVATE magic_args)
  if (APPLE)
    target_link_libraries("${TARGET}" PRIVATE Iconv::Iconv)
  endif ()

  foreach (LIBRARY IN LISTS ARG_LIBRARIES)
    target_link_libraries("${TARGET}" PRIVATE "${LIBRARY}")
  endforeach ()

  return(PROPAGATE SUBCOMMAND_EXAMPLES)
endfunction()

add_example(example-minimal minimal.cpp)
add_example(example-minimal-powershell-style minimal-powershell-style.cpp)
add_example(example-everything everything.cpp)
add_example(example-subcommands subcommands.cpp)
add_example(example-multicall SUBCOMMANDS multicall.cpp)
add_example(example-multicall-powershell-style SUBCOMMANDS multicall-powershell-style.cpp)

if (WIN32)
  add_library(win32-utf8-messagebox OBJECT win32_utf8_messagebox.cpp win32_utf8_messagebox.hpp)
  target_compile_features(win32-utf8-messagebox PUBLIC cxx_std_23)
  add_example(
    example-winmain
    WIN32
    winmain.cpp
    LIBRARIES
    win32-utf8-messagebox
  )
  add_example(
    example-winmain-subcommands
    WIN32 SUBCOMMANDS
    winmain-subcommands.cpp
    LIBRARIES
    win32-utf8-messagebox
  )
  add_example(
    example-winmain-multicall
    WIN32 SUBCOMMANDS
    winmain-multicall.cpp
    LIBRARIES
    win32-utf8-messagebox
  )
endif ()

# The cmake command needs the 'enumerate-subcommands' utility
if (BUILD_UTILITIES)
  include(MagicArgs)
  foreach (TARGET IN LISTS SUBCOMMAND_EXAMPLES)
    magic_args_enumerate_subcommands(
      "${TARGET}"
      HARDLINKS_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_NAME:${TARGET}>-links"
    )
  endforeach ()
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
