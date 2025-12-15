// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/gnu_style_parsing_traits.hpp>
#include "subcommands/declarations.hpp"
#endif

namespace magic_args::detail {
template <
  parsing_traits Traits,
  subcommand First,
  subcommand... Rest,
  class TExpected>
void parse_subcommands_silent_impl(
  std::optional<TExpected>& result,
  std::string_view command,
  argv_range auto&& argv,
  const program_info& help) {
  if (std::string_view {First::name} != command) {
    if constexpr (sizeof...(Rest) > 0) {
      parse_subcommands_silent_impl<Traits, Rest...>(
        result, command, argv, help);
    }
    return;
  }

  // Skip argv[0] and argv[1], instead of just argv[0]
  struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};
  auto subcommandResult
    = parse_silent<typename First::arguments_type, InnerTraits>(argv, help);
  if (subcommandResult) [[likely]] {
    result.emplace(
      subcommand_match<First>(std::move(subcommandResult).value()));
  } else {
    result.emplace(
      std::unexpect,
      incomplete_subcommand_parse_reason_t<First>(
        std::move(subcommandResult).error()));
  }
}
}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <
  parsing_traits Traits,
  subcommand First,
  subcommand... Rest,
  class TSuccess
  = std::variant<subcommand_match<First>, subcommand_match<Rest>...>,
  class TIncomplete = std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected parse_subcommands_silent(
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

  std::optional<TExpected> result;
  detail::parse_subcommands_silent_impl<Traits, First, Rest...>(
    result, command, argv, help);
  if (result) [[likely]] {
    return std::move(result).value();
  }

  return std::unexpected {invalid_argument_value {
        .mSource = {
          .mArgvSlice = std::vector { std::string { command } },
          .mName = "COMMAND",
          .mValue = std::string { command },
        },
      }};
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  return parse_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv), help);
}

template <class... Args>
auto parse_subcommands_silent(
  const int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands_silent<Args...>(
    std::views::counted(argv, argc), help, outputStream, errorStream);
}
}// namespace magic_args::inline public_api
