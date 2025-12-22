add_executable(list-subcommands list-subcommands.cpp)
target_compile_features(list-subcommands PRIVATE cxx_std_23)
set_target_properties(
  list-subcommands
  PROPERTIES
  OUTPUT_NAME "magic_args-list-subcommands"
)

add_executable(magic_args::list-subcommands ALIAS list-subcommands)

if (WIN32)
  target_sources(list-subcommands PRIVATE list-subcommands_win32.hpp)
else ()
  include(CheckIncludeFileCXX)
  check_include_file_cxx("elf.h" HAVE_ELF_H)
  if (HAVE_ELF_H)
    target_sources(list-subcommands PRIVATE list-subcommands_elf.hpp)
  else ()
    target_sources(list-subcommands PRIVATE list-subcommands_dlfcn.hpp)
    target_link_libraries(list-subcommands PRIVATE "${CMAKE_DL_LIBS}")
  endif ()
  target_link_libraries(list-subcommands PRIVATE magic_args)
endif ()

install(
  TARGETS
  list-subcommands
  EXPORT exports
  RUNTIME DESTINATION bin
)

if (BUILD_TESTING AND NOT BUILD_EXAMPLES)
  message(WARNING "Not testing utilities - examples must be built too")
endif ()
if (NOT (BUILD_TESTING AND BUILD_EXAMPLES))
  return()
endif ()

message(STATUS "Enabling utility tests")

add_test(NAME test-inspect-subcommands
  COMMAND list-subcommands $<TARGET_FILE:example-multicall>
)
set_tests_properties(test-inspect-subcommands PROPERTIES
  PASS_REGULAR_EXPRESSION "foo\nherp\n"
)

add_test(NAME test-inspect-no-subcommands
  COMMAND list-subcommands $<TARGET_FILE:example-minimal>
)
set_tests_properties(
  test-inspect-no-subcommands
  PROPERTIES
  PASS_REGULAR_EXPRESSION "^[[:space:]]*$"
  # Non-zero exit code is expected
  WILL_FAIL true
)