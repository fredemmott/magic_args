// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool foo {false};
  std::string bar;
  int baz {0};
};

int main(int argc, char** argv) {
  // This gets you an:
  //
  // ```
  // std::expected<
  //   MyArgs,
  //   magic_args::incomplete_parse_reason_t
  // >
  // ```
  //
  // WARNING: this assumes UTF-8 argv. See `minimal-with-encoding.cpp` for
  // a less-trusting example.
  const auto args = magic_args::parse<MyArgs>(std::views::counted(argv, argc));
  if (args) {
    magic_args::dump(*args);
    return EXIT_SUCCESS;
  }

  // This could be an actual error, e.g. invalid argument,
  // or something like `--help` or `--version`, which while not an error,
  // are an 'unexpected' outcome in the std::expected
  return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
}