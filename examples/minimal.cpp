// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  int mBaz {0};
};

int main(int argc, char** argv) {
  const std::expected<MyArgs, magic_args::incomplete_parse_reason> args
    = magic_args::parse<MyArgs>(argc, argv);
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