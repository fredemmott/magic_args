// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <magic_args/subcommands.hpp>

struct CommandFooBar {
  static constexpr auto name = "foo";
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
  };
  static int main(arguments_type&& args) {
    std::println("in CommandFooBar::main");
    magic_args::dump(args);
    return EXIT_SUCCESS;
  }
};

struct CommandHerp {
  static constexpr auto name = "herp";

  struct arguments_type {
    static constexpr auto description = "Do the derpy thing";
    static constexpr auto version = "Herp v1.2.3";

    std::string mDerp;
  };

  static int main(arguments_type&& args) {
    std::println("in CommandFooBar::main");
    magic_args::dump(args);
    return EXIT_SUCCESS;
  }
};

MAGIC_ARGS_SUBCOMMANDS_MAIN(CommandFooBar, CommandHerp);