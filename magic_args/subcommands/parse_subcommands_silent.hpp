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
  const program_info& info) {
  if (std::string_view {First::name} != command) {
    if constexpr (sizeof...(Rest) > 0) {
      parse_subcommands_silent_impl<Traits, Rest...>(
        result, command, argv, info);
    }
    return;
  }

  // Skip argv[0] and argv[1], instead of just argv[0]
  struct SubcommandTraits
    : Traits,
      detail::skip_args_count_trait<detail::skip_args_count<Traits>() + 1> {};
  const auto subcommandInfo = [&] {
    if constexpr (subcommand_with_info<First>) {
      return First::subcommand_info();
    } else {
      static_assert(subcommand<First>);
      return std::reference_wrapper {info};
    }
  }();

  auto subcommandResult
    = parse_silent<typename First::arguments_type, SubcommandTraits>(
      argv, subcommandInfo);
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
  class TIncomplete = incomplete_command_parse_reason_t<First, Rest...>,
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
  const char* const* argv,
  const program_info& help = {}) {
  return parse_subcommands_silent<Args...>(
    std::views::counted(argv, argc), help);
}
}// namespace magic_args::inline public_api
