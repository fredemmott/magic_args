function(add_example TARGET)
  add_executable("${TARGET}" "${ARGN}")
  target_link_libraries("${TARGET}" PRIVATE magic_args)
endfunction()

add_example(minimal minimal.cpp)
add_example(minimal-powershell-style minimal-powershell-style.cpp)