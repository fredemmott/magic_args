// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <magic_args/magic_args.hpp>

enum class OutputStyle {
  List,
  Quiet,
  CMakeInstall,
};

std::expected<void, magic_args::invalid_argument_value> from_argument_value(
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

auto to_argument_value(const OutputStyle& m) {
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
    .help = "Overwrite link if it already exists",
    .short_name = "f",
  };

  magic_args::option<std::string> mTextFile {
    .help = "Write list to text file; you might also want --quiet",
  };
  magic_args::option<std::string> mSymlinks {
    .help = "Create symlinks in this directory",
  };
  magic_args::flag mRelativeSymlinks {
    .help
    = "Create symlinks with a relative path to EXECUTABLE, instead of an "
      "absolute path",
  };
  magic_args::option<std::string> mHardlinks {
    .help = "Create hard links in this directory",
  };
  magic_args::option<std::string> mStampFile {
    .help
    = "File to create/touch every time the command completes without failure",
  };

  OutputStyle mOutputStyle {};

  magic_args::mandatory_positional_argument<std::string> mExecutable {
    .help = "A magic_args subcommands executable to inspect",
  };
};
