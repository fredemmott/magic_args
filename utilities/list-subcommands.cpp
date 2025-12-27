// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <fstream>
#include <print>

#include "list-subcommands_config.hpp"

#ifdef _WIN32
#include "list-subcommands_win32.hpp"
#elif __has_include(<elf.h>)
#include "list-subcommands_elf.hpp"
#else
#include "list-subcommands_dlfcn.hpp"
#endif

namespace fs = std::filesystem;

struct arguments {
  magic_args::option<std::string> mTextFile {
    .mHelp = "Write list to text file; you might also want --quiet",
  };
  magic_args::option<std::string> mSymlinks {
    .mHelp = "Create symlinks in this directory",
  };
  magic_args::option<std::string> mHardlinks {
    .mHelp = "Create hard links in this directory",
  };
  magic_args::option<std::string> mStampFile {
    .mHelp
    = "File to create/touch every time the command completes without failure",
  };

  magic_args::mandatory_positional_argument<std::string> mExecutable {
    .mHelp = "A magic_args subcommands executable to inspect",
  };

  magic_args::flag mQuiet {
    .mHelp = "Disable stdout output",
    .mShortName = "q",
  };
};

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

enum class LinkKind {
  Symlink,
  Hardlink,
};

template <LinkKind K>
std::expected<void, int> create_link(
  const fs::path& target,
  const fs::path& root,
  const std::string_view name) try {
  if (root.empty()) {
    return {};
  }
  const auto link
    = root / std::format("{}{}", name, BuildConfig::ExecutableSuffix);
  if (exists(link)) {
    fs::remove(link);
  }

  if constexpr (K == LinkKind::Symlink) {
    fs::create_symlink(target, link);
  } else {
    fs::create_hard_link(target, link);
  }
  return {};
} catch (const fs::filesystem_error& e) {
  std::println(
    stderr,
    "Failed to create {} link `{}/{}{}`: {}",
    K == LinkKind::Symlink ? "symlink" : "hardlink",
    root.string(),
    name,
    BuildConfig::ExecutableSuffix,
    e.what());
  return std::unexpected {EXIT_FAILURE};
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

  std::unique_ptr<std::FILE, decltype(&std::fclose)> textFile(
    nullptr, &std::fclose);

  if (!args.mTextFile->empty()) {
    const auto error
      = fopen_s(std::out_ptr(textFile), args.mTextFile->c_str(), "wb");
    if (error) {
      std::println(stderr, "Failed to open `{}`: {}", *args.mTextFile, error);
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

  auto begin = *dataPtr;
  while (*begin) {
    const std::string_view view {begin};
    begin += view.size() + 1;
    if (!args.mQuiet) {
      std::println(stdout, "{}", view);
    }
    if (textFile) {
      std::println(textFile.get(), "{}", view);
    }

    if (const auto ok
        = create_link<LinkKind::Symlink>(executablePath, symlinksPath, view);
        !ok) {
      return ok.error();
    }
    if (const auto ok
        = create_link<LinkKind::Hardlink>(executablePath, hardlinksPath, view);
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
