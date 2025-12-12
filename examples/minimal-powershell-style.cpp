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
    if (const auto& e = args.error();
        holds_alternative<magic_args::help_requested>(e)
        || holds_alternative<magic_args::version_requested>(e)) {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}