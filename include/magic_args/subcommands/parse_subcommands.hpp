// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_PARSE_SUBCOMMANDS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_PARSE_SUBCOMMANDS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "parse_subcommands_silent.hpp"
#include "print_incomplete_command_parse_reason.hpp"
#endif

namespace magic_args::inline public_api {

template <root_command_traits Traits, subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_subcommands_silent<Traits, First, Rest...>(argv);
  if (ret) [[likely]] {
    return ret;
  }

  std::visit(
    detail::overloaded {
      [&]<incomplete_parse_reason T>(const T& reason) {
        detail::print_incomplete_command_parse_reason<Traits, First, Rest...>(
          reason, argv, outputStream, errorStream);
        if constexpr (T::is_error) {
          detail::print(errorStream, "\n\n");
          detail::show_command_usage<Traits, First, Rest...>(argv, errorStream);
        }
      },
      [&]<subcommand T>(const incomplete_subcommand_parse_reason_t<T>& reason) {
        using ParsingTraits = detail::root_command_parsing_traits<Traits>::type;
        using SubcommandArgs = typename T::arguments_type;
        using SubcommandTraits
          = detail::subcommand_parsing_traits_t<ParsingTraits, SubcommandArgs>;
        detail::print_incomplete_parse_reason<SubcommandTraits, SubcommandArgs>(
          reason.value(), argv, outputStream, errorStream);
      },
    },
    ret.error());
  return ret;
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<gnu_style_parsing_traits, First, Rest...>(
    argv, outputStream, errorStream);
}

}// namespace magic_args::inline public_api

#endif
