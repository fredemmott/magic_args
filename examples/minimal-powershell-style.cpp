// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  using parsing_traits = magic_args::powershell_style_parsing_traits;

  bool foo {false};
  std::string bar;
  std::string baz;
};

MAGIC_ARGS_MAIN(MyArgs&& args) {
  magic_args::dump(args);
  return EXIT_SUCCESS;
}