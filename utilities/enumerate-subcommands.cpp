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

enum class OutputStyle {
  List,
  Quiet,
  CMakeInstall,
};

std::expected<void, magic_args::invalid_argument_value> from_string_argument(
  OutputStyle& m,
  std::string_view s) {
  if (s == "list") {
    m = OutputStyle::List;
    return {};
  }
  if (s == "quiet") {
    m = OutputStyle::Quiet;
    return {};
  }
  if (s == "cmake-install") {
    m = OutputStyle::CMakeInstall;
    return {};
  }
  return std::unexpected {magic_args::invalid_argument_value {}};
}
auto formattable_argument_value(const OutputStyle& m) {
  using enum OutputStyle;
  switch (m) {
    case List:
      return "list";
    case Quiet:
      return "quiet";
    case CMakeInstall:
      return "cmake-install";
  }
  std::unreachable();
}

struct arguments {
  magic_args::flag mForce {
    .mHelp = "Overwrite link if it already exists",
    .mShortName = "f",
  };

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

  OutputStyle mOutputStyle {};

  magic_args::mandatory_positional_argument<std::string> mExecutable {
    .mHelp = "A magic_args subcommands executable to inspect",
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

enum class [[nodiscard]] LinkState {
  Absent,
  Differs,
  UpToDate,
};

template <class Derived>
struct symlink_t {
  static constexpr auto kind = "symlink";

  static constexpr LinkState state(
    const fs::path& target,
    const fs::path& link) {
    switch (const auto stat = symlink_status(link); stat.type()) {
      case fs::file_type::not_found:
        return LinkState::Absent;
      case fs::file_type::symlink:
        if (read_symlink(link) != Derived::normalize_target(target, link)) {
          return LinkState::Differs;
        }
        return LinkState::UpToDate;
        break;
      default:
        return LinkState::Differs;
    }
  }

  static constexpr void create(const fs::path& target, const fs::path& link) {
    fs::create_symlink(Derived::normalize_target(target, link), link);
  }
};
struct absolute_symlink_t : symlink_t<absolute_symlink_t> {
  static fs::path normalize_target(const fs::path& target, const fs::path&) {
    return fs::canonical(target);
  }
};
struct relative_symlink_t : symlink_t<relative_symlink_t> {
  static fs::path normalize_target(
    const fs::path& target,
    const fs::path& link) {
    return fs::relative(target, link.parent_path());
  }
};
struct hardlink_t {
  static constexpr auto kind = "hardlink";
  static void create(const fs::path& target, const fs::path& link) {
    fs::create_hard_link(target, link);
  }
  static constexpr LinkState state(
    const fs::path& target,
    const fs::path& link) {
    switch (symlink_status(link).type()) {
      case fs::file_type::not_found:
        return LinkState::Absent;
      case fs::file_type::regular:
        return fs::equivalent(target, link) ? LinkState::UpToDate
                                            : LinkState::Differs;
      default:
        return LinkState::Differs;
    }
  }
};

template <class T>
concept link_kind = requires(const fs::path& p) {
  { T::kind } -> std::formattable<char>;
  T::create(p, p);
  { T::state(p, p) } -> magic_args::detail::same_as_ignoring_cvref<LinkState>;
};

template <link_kind TLink>
std::expected<void, int> create_link(
  const fs::path& target,
  const fs::path& root,
  const std::string_view name,
  const arguments& args) try {
  if (root.empty()) {
    return {};
  }

  const auto link
    = root / std::format("{}{}", name, BuildConfig::ExecutableSuffix);
  // Don't use fs::canonical because that follows symlinks
  const auto normalizedLink = fs::absolute(link.lexically_normal());
  switch (TLink::state(target, link)) {
    case LinkState::UpToDate:
      if (args.mOutputStyle == OutputStyle::CMakeInstall) {
        std::println("-- Up-to-date: {}", normalizedLink.string());
      }
      return {};
    case LinkState::Absent:
      break;
    case LinkState::Differs:
      if (args.mForce) {
        fs::remove(link);
      } else {
        std::println(
          stderr,
          "File already exists; use `--force` to overwrite: {}",
          normalizedLink.string());
        return std::unexpected {EXIT_FAILURE};
      }
      break;
  }

  if (args.mOutputStyle == OutputStyle::CMakeInstall) {
    std::println("-- Installing: {}", normalizedLink.string());
  }

  TLink::create(target, link);
  return {};
} catch (const fs::filesystem_error& e) {
  std::println(
    stderr,
    "Failed to create {} link `{}/{}{}`: {}",
    TLink::kind,
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
