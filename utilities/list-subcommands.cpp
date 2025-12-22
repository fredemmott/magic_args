// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#ifdef _WIN32
#include "list-subcommands_win32.hpp"
#elif __has_include(<elf.h>)
#include "list-subcommands_elf.hpp"
#else
#include "list-subcommands_dlfcn.hpp"
#endif

#include <print>

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  if (argc != 2) {
    std::println(
      stderr, "You must pass the path to an executable as the only parameter");
    return EXIT_FAILURE;
  }
  const std::filesystem::path path {argv[1]};
  if (std::error_code ec; !std::filesystem::exists(path, ec)) {
    std::println(
      stderr, "`{}` does not exist: {}", path.string(), ec.message());
    return EXIT_FAILURE;
  }

  const auto lib = open_library(path.wstring());
  if (!lib) {
    std::println(stderr, "Failed to open `{}`: {}", path.string(), lib.error());
    return EXIT_FAILURE;
  }

  const auto dataPtr = get_data_pointer(*lib);
  if (!dataPtr) {
    std::println(
      stderr,
      "Executable `{}` does not contain subcommands inspection data: {}",
      path.string(),
      dataPtr.error());
    return EXIT_FAILURE;
  }

  auto begin = *dataPtr;
  while (*begin) {
    const std::string_view view {begin};
    begin += view.size() + 1;
    std::println(stdout, "{}", view);
  }

  return EXIT_SUCCESS;
}
