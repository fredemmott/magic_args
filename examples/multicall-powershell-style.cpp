// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#if __has_include(<magic_args/subcommands.hpp>)
#include <magic_args/subcommands.hpp>
#endif

struct CommandFooBar {
  static constexpr auto name = "foo";
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
  };

  static int main(arguments_type&& args) {
    std::println("CommandFooBar::Main()");
    magic_args::dump(args);
    return 123;
  }
};

struct CommandHerp {
  static constexpr auto name = "herp";
  struct arguments_type {
    std::string mDerp;
  };

  static int main(arguments_type&& args) {
    std::println("CommandHerp::Main()");
    magic_args::dump(args);
    return 456;
  }
};

struct RootInfo {
  using parsing_traits = magic_args::powershell_style_parsing_traits;
  static constexpr auto description = "PowerShell-style multicall thing";
};

// Invoke as `foo` or `herp`, *not* `example-multicall foo`
MAGIC_ARGS_MULTI_CALL_MAIN(RootInfo, CommandFooBar, CommandHerp);