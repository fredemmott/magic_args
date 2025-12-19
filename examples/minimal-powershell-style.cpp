// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  using parsing_traits = magic_args::powershell_style_parsing_traits;

  bool foo {false};
  std::string bar;
  std::string baz;
};

int main(int argc, char** argv) {
  const auto args = magic_args::parse<MyArgs>(std::views::counted(argv, argc));
  if (!args) {
    // This could be an actual error, e.g. invalid argument,
    // or something like `--help` or `--version`, which while not an error,
    // are an 'unexpected' outcome in the std::expected
    return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}