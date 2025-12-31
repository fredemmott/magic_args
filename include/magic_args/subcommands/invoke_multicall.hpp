// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_MULTICALL_HPP
#define MAGIC_ARGS_SUBCOMMANDS_MULTICALL_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/main_macros.hpp>
#include "subcommands/inspection.hpp"
#include "subcommands/invoke_subcommands.hpp"
#include "subcommands/is_error.hpp"
#endif

namespace magic_args::inline public_api {
template <parsing_traits T = gnu_style_parsing_traits>
struct multicall_traits : T {
  static constexpr std::size_t skip_args_count = 0;

  static std::string_view command_from_argument(std::string_view arg) {
    const auto directorySeparator = arg.find_last_of("/\\");
    if (directorySeparator != std::string_view::npos) {
      arg.remove_prefix(directorySeparator + 1);
    }

    const auto extensionSeparator = arg.find_last_of('.');
    switch (extensionSeparator) {
      case std::string_view::npos:
        return arg;
      case 0:// .foo -> foo
        return arg.substr(1);
      default:
        return arg.substr(0, extensionSeparator);
    }
  }
};

template <
  class First,
  class... Rest,
  detail::argv_range TArgv,
  console_output TConsole = print_console_output>
auto invoke_multicall(TArgv&& argv, TConsole&& console = {}) {
  using RootTraits
    = std::conditional_t<root_command_traits<First>, First, detail::empty_t>;
  using ParsingTraits =
    typename detail::root_command_parsing_traits_t<RootTraits>;
  struct NextRootTraits : RootTraits {
    using parsing_traits = multicall_traits<ParsingTraits>;
  };
  static_assert(root_command_traits<NextRootTraits>);
  static_assert(has_parsing_traits<NextRootTraits>);

  if constexpr (root_command_traits<First>) {
    return invoke_subcommands<NextRootTraits, Rest...>(
      std::forward<TArgv>(argv), std::forward<TConsole>(console));
  } else {
    return invoke_subcommands<NextRootTraits, First, Rest...>(
      std::forward<TArgv>(argv), std::forward<TConsole>(console));
  }
}

template <class... Ts, detail::argv_range TArgv>
auto invoke_multicall_silent(TArgv&& argv) {
  return invoke_multicall<Ts...>(
    std::forward<TArgv>(argv), drop_console_output {});
}

}// namespace magic_args::inline public_api

#endif