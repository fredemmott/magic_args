// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/program_info.hpp>
#include "parse_subcommands_silent.hpp"
#include "print_incomplete_command_parse_reason.hpp"
#endif

namespace magic_args::inline public_api {

template <parsing_traits Traits, subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& info = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_subcommands_silent<Traits, First, Rest...>(argv, info);
  if (ret) [[likely]] {
    return ret;
  }

  // Skip over argv[1], as well as argv[0]
  // 2025-12-14: VS2022 requires that this be declared outside the lambda
  struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};

  std::visit(
    detail::overloaded {
      [&]<incomplete_parse_reason T>(const T& reason) {
        detail::print_incomplete_command_parse_reason<Traits, First, Rest...>(
          reason, info, argv, outputStream, errorStream);
        if constexpr (T::is_error) {
          detail::print(errorStream, "\n\n");
          detail::show_command_usage<Traits, First, Rest...>(
            info, argv, errorStream);
        }
      },
      [&]<subcommand_with_info T>(
        const incomplete_subcommand_parse_reason_t<T>& reason) {
        detail::print_incomplete_parse_reason<
          typename T::arguments_type,
          InnerTraits>(
          reason.value(),
          T::subcommand_info(),
          argv,
          outputStream,
          errorStream);
      },
      [&]<subcommand T>(const incomplete_subcommand_parse_reason_t<T>& reason) {
        detail::print_incomplete_parse_reason<
          typename T::arguments_type,
          InnerTraits>(reason.value(), info, argv, outputStream, errorStream);
      },
    },
    ret.error());
  return ret;
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& info = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<gnu_style_parsing_traits, First, Rest...>(
    argv, info, outputStream, errorStream);
}

template <class... Args>
auto parse_subcommands(
  const int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<Args...>(
    std::views::counted(argv, argc), help, outputStream, errorStream);
}

}// namespace magic_args::inline public_api
