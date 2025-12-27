set(
  LIST_EXECUTABLES_CONFIG_HPP
  "${CMAKE_CURRENT_BINARY_DIR}/include/enumerate-subcommands_config.hpp"
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

add_executable(enumerate-subcommands enumerate-subcommands.cpp "${LIST_EXECUTABLES_CONFIG_HPP}")
target_compile_features(enumerate-subcommands PRIVATE cxx_std_23)
target_include_directories(enumerate-subcommands PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")
set_target_properties(
  enumerate-subcommands
  PROPERTIES
  OUTPUT_NAME "magic_args-enumerate-subcommands"
)
target_link_libraries(
  enumerate-subcommands
  PRIVATE
  magic_args::magic_args
)

add_executable(magic_args::enumerate-subcommands ALIAS enumerate-subcommands)

if (WIN32)
  target_sources(enumerate-subcommands PRIVATE enumerate-subcommands_win32.hpp)
else ()
  include(CheckIncludeFileCXX)
  check_include_file_cxx("elf.h" HAVE_ELF_H)
  if (HAVE_ELF_H)
    target_sources(enumerate-subcommands PRIVATE enumerate-subcommands_elf.hpp)
  else ()
    target_sources(enumerate-subcommands PRIVATE enumerate-subcommands_dlfcn.hpp)
    target_link_libraries(enumerate-subcommands PRIVATE "${CMAKE_DL_LIBS}")
  endif ()
  target_link_libraries(enumerate-subcommands PRIVATE magic_args)
endif ()

if (APPLE)
  find_library(ICONV_LIBRARY iconv REQUIRED)
  target_link_libraries(enumerate-subcommands PRIVATE "${ICONV_LIBRARY}")
endif ()

install(
  TARGETS
  enumerate-subcommands
  EXPORT exports
  RUNTIME DESTINATION bin
)