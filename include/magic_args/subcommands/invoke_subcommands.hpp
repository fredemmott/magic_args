// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "invoke_subcommands_silent.hpp"
#include "parse_subcommands.hpp"
#endif

namespace magic_args::inline public_api {

template <
  root_command_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class TSuccess = std::
    invoke_result_t<decltype(First::main), typename First::arguments_type&&>,
  class TIncomplete = incomplete_command_parse_reason_t<First, Rest...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected invoke_subcommands(
  detail::argv_range auto&& argv,
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  auto result = parse_subcommands<Traits, First, Rest...>(
    argv, outputStream, errorStream);
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
  compatible_invocable_subcommand<First>... Rest>
auto invoke_subcommands(
  detail::argv_range auto&& argv,
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return invoke_subcommands<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv), outputStream, errorStream);
}

}// namespace magic_args::inline public_api

#endif
