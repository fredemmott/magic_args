// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_SILENT_HPP
#define MAGIC_ARGS_SUBCOMMANDS_INVOKE_SUBCOMMANDS_SILENT_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "invocable_declarations.hpp"
#include "parse_subcommands_silent.hpp"
#endif

#include <expected>
#include <functional>

namespace magic_args::inline public_api {
template <
  root_command_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class TSuccess = std::
    invoke_result_t<decltype(First::main), typename First::arguments_type&&>,
  class TIncomplete = incomplete_command_parse_reason_t<First, Rest...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected invoke_subcommands_silent(detail::argv_range auto&& argv) {
  auto result = parse_subcommands_silent<Traits, First, Rest...>(argv);
  if (!result) [[unlikely]] {
    return std::unexpected {std::move(result).error()};
  }

  if constexpr (std::is_void_v<std::invoke_result_t<
                  decltype(First::main),
                  typename First::arguments_type&&>>) {
    std::visit(
      []<class T>(subcommand_match<T>&& match) {
        std::invoke(T::main, std::move(match).value());
      },
      std::move(result).value());
    return {};
  } else {
    return std::visit(
      []<class T>(subcommand_match<T>&& match) {
        return std::invoke(T::main, std::move(match).value());
      },
      std::move(result).value());
  }
}

// Convenience helper, assuming gnu_style_parsing_traits
template <
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest>
auto invoke_subcommands_silent(detail::argv_range auto&& argv) {
  return invoke_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv));
}

}// namespace magic_args::inline public_api

#endif
