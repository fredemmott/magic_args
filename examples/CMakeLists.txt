function(add_example TARGET)
  add_executable("${TARGET}" ${ARGN})
  target_link_libraries("${TARGET}" PRIVATE magic_args)
endfunction()

add_example(example-minimal minimal.cpp)
add_example(example-minimal-powershell-style minimal-powershell-style.cpp)
add_example(example-everything everything.cpp)
add_example(example-subcommands subcommands.cpp)

if (WIN32)
  add_example(example-windows WIN32 windows.cpp "${CMAKE_SOURCE_DIR}/tests/utf8-process-code-page.manifest")
  target_compile_definitions(
    example-windows
    PRIVATE
    UNICODE
    _UNICODE
  )
  if (MSVC)
    target_compile_options(
      example-windows
      PRIVATE
      /utf-8
    )
  endif ()
endif ()