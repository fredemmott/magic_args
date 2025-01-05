// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  int mBaz {0};
};

int main(int argc, char** argv) {
  const auto args = magic_args::parse<MyArgs>(argc, argv);
  if (!args.has_value()) {
    if (args.error() == magic_args::HelpRequested) {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}