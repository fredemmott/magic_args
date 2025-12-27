// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "enumerate-subcommands_arguments.hpp"
#include "enumerate-subcommands_create_links.hpp"

#ifdef _WIN32
#include "enumerate-subcommands_win32.hpp"
#elif __has_include(<elf.h>)
#include "enumerate-subcommands_elf.hpp"
#else
#include "enumerate-subcommands_dlfcn.hpp"
#endif

#include <fstream>
#include <print>

#ifdef _MSC_VER
#define MSVC_PUSH_DISABLE_WARNING(N) \
  __pragma(warning(push)) __pragma(warning(disable : N))
#define MSVC_POP_WARNINGS() __pragma(warning(pop))
#else
#define MSVC_PUSH_DISABLE_WARNING(N)
#define MSVC_POP_WARNINGS()
#endif

namespace fs = std::filesystem;

namespace {
void ensure_directory_or_empty_string(const fs::path& path) {
  if (path.empty()) {
    return;
  }

  if (exists(path) && is_directory(path)) {
    return;
  }
  fs::create_directories(path);
}

}// namespace

MAGIC_ARGS_MAIN(const arguments& args) {
  const fs::path executablePath {args.mExecutable.value()};
  if (std::error_code ec; !fs::exists(executablePath, ec)) {
    std::println(
      stderr, "`{}` does not exist: {}", executablePath.string(), ec.message());
    return EXIT_FAILURE;
  }

  const auto lib = open_library(executablePath.wstring());
  if (!lib) {
    std::println(
      stderr, "Failed to open `{}`: {}", executablePath.string(), lib.error());
    return EXIT_FAILURE;
  }

  const auto dataPtr = get_data_pointer(*lib);
  if (!dataPtr) {
    std::println(
      stderr,
      "Executable `{}` does not contain subcommands inspection data: {}",
      executablePath.string(),
      dataPtr.error());
    return EXIT_FAILURE;
  }

  // GCC has non-standard attributes on fclose, and gets upset if they're
  // lost by using `decltype(fclose)`
  constexpr auto fileDeleter = [](FILE* f) { return fclose(f); };
  std::unique_ptr<std::FILE, decltype(fileDeleter)> textFile(
    nullptr, fileDeleter);

  if (!args.mTextFile->empty()) {
    MSVC_PUSH_DISABLE_WARNING(4996)
    textFile.reset(std::fopen(args.mTextFile->c_str(), "wb"));
    MSVC_POP_WARNINGS()
    if (!textFile) {
      std::println(stderr, "Failed to open `{}`: {}", *args.mTextFile, errno);
      return EXIT_FAILURE;
    }
  }

  const fs::path hardlinksPath {*args.mHardlinks};
  const fs::path symlinksPath {*args.mSymlinks};

  try {
    ensure_directory_or_empty_string(hardlinksPath);
  } catch (const fs::filesystem_error& e) {
    std::println(
      stderr,
      "Failed to create directory `{}`: {}",
      *args.mHardlinks,
      e.what());
    return EXIT_FAILURE;
  }
  try {
    ensure_directory_or_empty_string(symlinksPath);
  } catch (const fs::filesystem_error& e) {
    std::println(
      stderr, "Failed to create directory `{}`: {}", *args.mSymlinks, e.what());
    return EXIT_FAILURE;
  }

  const auto& create_symlink = args.mRelativeSymlinks
    ? create_link<relative_symlink_t>
    : create_link<absolute_symlink_t>;

  auto begin = *dataPtr;
  while (*begin) {
    const std::string_view view {begin};
    begin += view.size() + 1;
    if (args.mOutputStyle == OutputStyle::List) {
      std::println(stdout, "{}", view);
    }
    if (textFile) {
      std::println(textFile.get(), "{}", view);
    }

    if (const auto ok
        = create_symlink(executablePath, symlinksPath, view, args);
        !ok) {
      return ok.error();
    }
    if (const auto ok
        = create_link<hardlink_t>(executablePath, hardlinksPath, view, args);
        !ok) {
      return ok.error();
    }
  }

  if (args.mStampFile->empty()) {
    return EXIT_SUCCESS;
  }

  const fs::path path {*args.mStampFile};

  try {
    std::filesystem::create_directories(path.parent_path());
    if (!exists(path)) {
      std::ignore = std::ofstream {path};
      return EXIT_SUCCESS;
    }
    fs::last_write_time(path, fs::file_time_type::clock::now());
    return EXIT_SUCCESS;
  } catch (const fs::filesystem_error& e) {
    std::println(
      stderr, "Failed to touch stamp file `{}`: {}", path.string(), e.what());
    return EXIT_FAILURE;
  }
}
