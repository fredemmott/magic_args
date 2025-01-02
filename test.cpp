// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include "magic_args/magic_args.hpp"

struct BasicArgs {
  std::string mFoo;
  int mBar;
  magic_args::option<std::string> mBaz {
    .mHelp = "do the bazzy thing",
    .mShortName = "b",
  };
  bool mFlag { false };
  bool mOtherFlag { false };
};

int main(int argc, char** argv) {
  magic_args::show_usage<BasicArgs>(stdout, argv[0]);
  return 0;
}
