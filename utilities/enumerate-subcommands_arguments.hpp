// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <magic_args/magic_args.hpp>

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
