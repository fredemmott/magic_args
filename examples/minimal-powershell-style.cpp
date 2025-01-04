// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  std::string mBaz;
};

int main(int argc, char** argv) {
  const auto args
    = magic_args::parse<MyArgs, magic_args::powershell_style_parsing_traits>(
      argc, argv);
  if (!args.has_value()) {
    if (args.error() == magic_args::HelpRequested) {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}