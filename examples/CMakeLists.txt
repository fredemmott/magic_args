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
add_example(example-invocable-subcommands invocable-subcommands.cpp)
add_example(example-minimal-with-encoding minimal-with-encoding.cpp)
add_example(example-multicall multicall.cpp)
add_example(example-multicall-powershell-style multicall-powershell-style.cpp)

add_custom_command(
  TARGET example-multicall POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:example-multicall>/multicall-links"
  COMMAND "${CMAKE_COMMAND}" -E create_hardlink
  "$<TARGET_FILE:example-multicall>"
  "$<TARGET_FILE_DIR:example-multicall>/multicall-links/foo.exe"
  COMMAND "${CMAKE_COMMAND}" -E create_hardlink
  "$<TARGET_FILE:example-multicall>"
  "$<TARGET_FILE_DIR:example-multicall>/multicall-links/herp.exe"
)

add_custom_command(
  TARGET example-multicall-powershell-style POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:example-multicall-powershell-style>/multicall-links-powershell-style"
  COMMAND "${CMAKE_COMMAND}" -E create_hardlink
  "$<TARGET_FILE:example-multicall-powershell-style>"
  "$<TARGET_FILE_DIR:example-multicall>/multicall-links-powershell-style/foo.exe"
  COMMAND "${CMAKE_COMMAND}" -E create_hardlink
  "$<TARGET_FILE:example-multicall-powershell-style>"
  "$<TARGET_FILE_DIR:example-multicall>/multicall-links-powershell-style/herp.exe"
)

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