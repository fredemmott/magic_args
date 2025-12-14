// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <magic_args/powershell_style_parsing_traits.hpp>

#include <ranges>

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
    std::string mDerp;
  };
};

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

int main(int argc, char** argv) {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::views::counted(argv, argc));
  if (!ret) {
    if (magic_args::is_error(ret.error())) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
  return 0;
}