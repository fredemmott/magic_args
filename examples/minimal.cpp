// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  // If omitted, defaults to gnu-style
  // using parsing_traits = magic_args::powershell_style_parsing_traits;
  // All optional
  static constexpr auto description = "Minimal example of magic_args";
  static constexpr auto version = "MyApp v1.2.3";
  static constexpr auto examples
    = std::array {"myapp -Foo=true", "myapp -Baz=42"};

  // Actual args
  bool foo {false};
  std::string bar;
  int baz {0};
};

MAGIC_ARGS_MAIN(const MyArgs& args) {
  magic_args::dump(args);
  return 0;
}