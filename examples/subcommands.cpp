// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

struct CommandFooBar {
  static constexpr auto name = "foo";
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
  };
};

struct CommandHerp {
  static constexpr auto name = "herp";
  static magic_args::program_info subcommand_info() noexcept {
    return {
      .mDescription = "Do the derpy thing",
      .mVersion = "Herp v1.2.3",
    };
  }

  struct arguments_type {
    std::string mDerp;
  };
};

int main(int argc, char** argv) {
  const auto ret
    = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(argc, argv);
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