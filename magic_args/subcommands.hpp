// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include "gnu_style_parsing_traits.hpp"
#endif
#include <concepts>
#include <expected>
#include <string_view>

namespace magic_args::inline public_api {

template <class T>
concept subcommand = requires(T v) {
  typename T::arguments_type;
  requires std::default_initializable<typename T::arguments_type>;
  { T::name } -> std::convertible_to<std::string_view>;
};

template <class T>
concept invocable_subcommand
  = subcommand<T> && std::invocable<T, typename T::arguments_type&&>;

template <
  subcommand T,
  class R
  = std::expected<typename T::arguments_type, incomplete_parse_reason_t>>
struct subcommand_match : R {
  using subcommand_type = T;

  subcommand_match() = delete;
  template <std::convertible_to<R> U>
  explicit subcommand_match(U&& v) : R {std::forward<U>(v)} {
  }
};

using incomplete_subcommand_parse_reason_t = std::variant<
  help_requested,
  version_requested,
  missing_required_argument,
  invalid_argument_value>;

template <parsing_traits Traits, subcommand First, subcommand... Rest>
std::expected<
  std::variant<subcommand_match<First>, subcommand_match<Rest>...>,
  incomplete_subcommand_parse_reason_t>
parse_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  if (argv.size() < 2) {
    return std::unexpected {missing_required_argument {
      .mSource = {.mName = "COMMAND"},
    }};
  }
  using CommonArguments = detail::common_arguments_t<Traits>;

  const std::string_view command {*(std::ranges::begin(argv) + 1)};
  if (
    command == CommonArguments::long_help
    || command == CommonArguments::short_help || command == "help") {
    return std::unexpected {help_requested {}};
  }
  if (command == CommonArguments::version && !help.mVersion.empty()) {
    return std::unexpected {version_requested {}};
  }

  if (std::string_view {First::name} != command) {
    if constexpr (sizeof...(Rest) > 0) {
      const auto result = parse_subcommands_silent<Traits, Rest...>(argv, help);
      if (!result) {
        return std::unexpected {result.error()};
      }
      return std::visit(
        []<typename T>(T&& v) { return std::forward<T>(v); },
        std::move(result).value());
    } else {
      return std::unexpected {invalid_argument_value {
        .mSource = {
          .mArgvSlice = std::vector { std::string { command } },
          .mName = "COMMAND",
          .mValue = std::string { command },
        },
      }};
    }
  }

  // TODO: thread through a 'start parsing at' offset instead of mutating
  // the view
  return subcommand_match<First>(
    parse_silent<typename First::arguments_type, Traits>(
      std::views::drop(argv, 1), help));
}

template <subcommand First, subcommand... Rest>
std::expected<
  std::variant<subcommand_match<First>, subcommand_match<Rest>...>,
  incomplete_subcommand_parse_reason_t>
parse_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  return parse_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv), help);
}

}// namespace magic_args::inline public_api