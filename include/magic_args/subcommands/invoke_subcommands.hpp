// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "invocable_declarations.hpp"
#include "parse_subcommands.hpp"
#endif

namespace magic_args::inline public_api {

template <
  root_command_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  console_output TConsole = print_console_output,
  class TSuccess = subcommand_return_t<First>,
  class TIncomplete = incomplete_command_parse_reason_t<First, Rest...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected invoke_subcommands(
  detail::argv_range auto&& argv,
  TConsole&& output = {}) {
  auto result = parse_subcommands<Traits, First, Rest...>(argv, output);
  if (!result) [[unlikely]] {
    return std::unexpected {std::move(result).error()};
  }

  return std::visit(
    []<class T>(subcommand_match<T>&& match) {
      if constexpr (std::is_void_v<TSuccess>) {
        std::invoke(T::main, std::move(match).value());
        return TExpected {};
      } else {
        return std::invoke(T::main, std::move(match).value());
      }
    },
    std::move(result).value());
}

template <
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  detail::argv_range TArgv,
  console_output TConsole = print_console_output>
auto invoke_subcommands(TArgv&& argv, TConsole&& console = {}) {
  return invoke_subcommands<gnu_style_parsing_traits, First, Rest...>(
    std::forward<TArgv>(argv), std::forward<TConsole>(console));
}

/** Invoke using a subcommands list from the root.
 *
 * e.g.
 *
 * ```
 * struct Root {
 *   using subcommands = magic_args::subcommands_list<Foo, Bar>;
 * };
 * ```
 */
template <
  root_command_traits Root,
  detail::argv_range TArgv,
  console_output TConsole = print_console_output>
  requires requires { typename Root::subcommands; }
auto invoke_subcommands(TArgv&& argv, TConsole&& console) {
  return [&]<
           invocable_subcommand First,
           compatible_invocable_subcommand<First>... Rest>(
           invocable_subcommands_list<First, Rest...>) {
    return invoke_subcommands<Root, First, Rest...>(
      std::forward<TArgv>(argv), std::forward<TConsole>(console));
  }(typename Root::subcommands {});
}

template <class... Ts>
auto invoke_subcommands_silent(detail::argv_range auto&& argv) {
  return invoke_subcommands<Ts...>(argv, drop_console_output {});
}

}// namespace magic_args::inline public_api

#endif
