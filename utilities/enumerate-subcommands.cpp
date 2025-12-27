// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <fstream>
#include <print>

#include "enumerate-subcommands_config.hpp"

#ifdef _WIN32
#include "enumerate-subcommands_win32.hpp"
#elif __has_include(<elf.h>)
#include "enumerate-subcommands_elf.hpp"
#else
#include "enumerate-subcommands_dlfcn.hpp"
#endif

#ifdef _MSC_VER
#define MSVC_PUSH_DISABLE_WARNING(N) \
  __pragma(warning(push)) __pragma(warning(disable : N))
#define MSVC_POP_WARNINGS() __pragma(warning(pop))
#else
#define MSVC_PUSH_DISABLE_WARNING(N)
#define MSVC_POP_WARNINGS()
#endif

namespace fs = std::filesystem;

struct arguments {
  magic_args::option<std::string> mTextFile {
    .mHelp = "Write list to text file; you might also want --quiet",
  };
  magic_args::option<std::string> mSymlinks {
    .mHelp = "Create symlinks in this directory",
  };
  magic_args::flag mRelativeSymlinks {
    .mHelp
    = "Create symlinks with a relative path to EXECUTABLE, instead of an "
      "absolute path",
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
  RelativeSymlink,
  AbsoluteSymlink,
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
  if (exists(link) || symlink_status(link).type() == fs::file_type::symlink) {
    fs::remove(link);
  }

  if constexpr (K == LinkKind::Hardlink) {
    fs::create_hard_link(target, link);
  } else if constexpr (K == LinkKind::AbsoluteSymlink) {
    fs::create_symlink(fs::canonical(target), link);
  } else {
    static_assert(K == LinkKind::RelativeSymlink);
    const auto relative = fs::relative(target, link.parent_path());
    fs::create_symlink(relative, link);
  }
  return {};
} catch (const fs::filesystem_error& e) {
  std::println(
    stderr,
    "Failed to create {} link `{}/{}{}`: {}",
    K == LinkKind::Hardlink ? "hardlink" : "symlink",
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
    ? create_link<LinkKind::RelativeSymlink>
    : create_link<LinkKind::AbsoluteSymlink>;

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

    if (const auto ok = create_symlink(executablePath, symlinksPath, view);
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
