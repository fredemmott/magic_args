// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool foo {false};
  std::string bar;
  int baz {0};
};

MAGIC_ARGS_MAIN(const MyArgs& args) {
  magic_args::dump(args);
  return 0;
}