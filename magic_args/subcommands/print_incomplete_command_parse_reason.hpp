// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/parse.hpp>
#include <magic_args/detail/print.hpp>
#include <magic_args/program_info.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::detail {

template <parsing_traits Traits, subcommand... Ts>
void show_command_usage(
  const program_info& info,
  argv_range auto&& argv,
  FILE* stream) {
  using CommonArguments = common_arguments_t<Traits>;
  detail::println(
    stream,
    "Usage: {} COMMAND [OPTIONS...]\n",
    get_prefix_for_user_messages<Traits>(argv));
  detail::println(stream, "Commands:\n");

  (
    [stream]<class T>(std::type_identity<T>) {
      if constexpr (subcommand_with_info<T>) {
        detail::println(
          stream, "      {:24} {}", T::name, T::subcommand_info().mDescription);
      } else {
        detail::println(stream, "      {}", T::name);
      }
    }(std::type_identity<Ts> {}),
    ...);

  detail::println(
    stream,
    "\n  {:2}, {:24} show this message",
    *CommonArguments::short_help,
    *CommonArguments::long_help);

  if (!info.mVersion.empty()) {
    detail::println(
      stream, "      {:24} print program version", *CommonArguments::version);
  }

  detail::println(
    stream,
    "\nFor more information, run:\n\n  {} COMMAND {}",
    get_prefix_for_user_messages<Traits>(argv),
    *CommonArguments::long_help);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const help_requested&,
  const program_info& info,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  show_command_usage<Traits, Ts...>(info, argv, outputStream);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const version_requested&,
  const program_info& info,
  argv_range auto&&,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  detail::println(outputStream, "{}", info.mVersion);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const missing_required_argument&,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: You must specify a COMMAND",
    get_prefix_for_user_messages<Traits>(argv));
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const invalid_argument_value& r,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: `{}` is not a valid COMMAND",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mValue);
}

}// namespace magic_args::detail
