if (APPLE)
  find_package(Iconv REQUIRED)
endif ()

function(add_example TARGET)
  add_executable("${TARGET}" ${ARGN})
  target_link_libraries("${TARGET}" PRIVATE magic_args)
  if (APPLE)
    target_link_libraries("${TARGET}" PRIVATE Iconv::Iconv)
  endif ()
endfunction()

add_example(example-minimal minimal.cpp)
add_example(example-minimal-powershell-style minimal-powershell-style.cpp)
add_example(example-everything everything.cpp)
add_example(example-subcommands subcommands.cpp)
add_example(example-multicall multicall.cpp)
add_example(example-multicall-powershell-style multicall-powershell-style.cpp)

# The cmake command needs the 'enumerate-subcommands' utility
if (BUILD_UTILITIES)
  include(MagicArgs)
  magic_args_enumerate_subcommands(
    example-multicall
    HARDLINKS_DIR "${CMAKE_CURRENT_BINARY_DIR}/multicall-links"
  )
  magic_args_enumerate_subcommands(
    example-multicall-powershell-style
    HARDLINKS_DIR "${CMAKE_CURRENT_BINARY_DIR}/multicall-powershell-style-links"
  )
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

if (WIN32)
  add_example(example-winmain WIN32 winmain.cpp "${CMAKE_SOURCE_DIR}/tests/utf8-process-code-page.manifest")
  target_compile_definitions(
    example-winmain
    PRIVATE
    UNICODE
    _UNICODE
  )
  if (MSVC)
    target_compile_options(
      example-winmain
      PRIVATE
      /utf-8
    )
  endif ()
endif ()