set(
  LIST_EXECUTABLES_CONFIG_HPP
  "${CMAKE_CURRENT_BINARY_DIR}/include/list-subcommands_config.hpp"
)
file(
  CONFIGURE
  OUTPUT "${LIST_EXECUTABLES_CONFIG_HPP}"
  CONTENT [=[
#pragma once
namespace {
namespace BuildConfig {
constexpr auto ExecutableSuffix = "@CMAKE_EXECUTABLE_SUFFIX@";
} // namespace BuildConfig
} // namespace
]=]
)

add_executable(list-subcommands list-subcommands.cpp "${LIST_EXECUTABLES_CONFIG_HPP}")
target_compile_features(list-subcommands PRIVATE cxx_std_23)
target_include_directories(list-subcommands PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
set_target_properties(
  list-subcommands
  PROPERTIES
  OUTPUT_NAME "magic_args-list-subcommands"
)
target_link_libraries(
  list-subcommands
  PRIVATE
  magic_args::magic_args
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

if (APPLE)
  find_library(ICONV_LIBRARY iconv REQUIRED)
  target_link_libraries(list-subcommands PRIVATE "${ICONV_LIBRARY}")
endif ()

install(
  TARGETS
  list-subcommands
  EXPORT exports
  RUNTIME DESTINATION bin
)