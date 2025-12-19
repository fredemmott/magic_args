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
};

struct CommandHerp {
  static constexpr auto name = "herp";

  struct arguments_type {
    static constexpr auto description = "Do the derpy thing";
    static constexpr auto version = "Herp v1.2.3";

    std::string mDerp;
  };
};

int main(int argc, char** argv) {
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    std::views::counted(argv, argc));
  if (!ret) {
    if (magic_args::is_error(ret.error())) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  std::visit(
    []<class T>(const magic_args::subcommand_match<T>& match) {
      std::println("Matched {}", T::name);
      magic_args::dump(*match);
    },
    ret.value());
  return 0;
}