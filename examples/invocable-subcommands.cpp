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

int main(int argc, char** argv) {
  const auto ret = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(
    std::views::counted(argv, argc));
  if (!ret) {
    // e.g. `--version`, `--help` aren't errors, but they are *unexpected*
    return magic_args::is_error(ret.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  std::println("Subcommand main returned {}", *ret);
  return 0;
}