// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_PRINT_INCOMPLETE_COMMAND_PARSE_REASON_HPP
#define MAGIC_ARGS_SUBCOMMANDS_PRINT_INCOMPLETE_COMMAND_PARSE_REASON_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/parse.hpp>
#include <magic_args/detail/print.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::detail {

template <root_command_traits RootTraits, subcommand... Ts>
void show_command_usage(argv_range auto&& argv, FILE* stream) {
  using ParsingTraits = root_command_parsing_traits_t<RootTraits>;
  using CommonArguments = common_arguments_t<ParsingTraits>;
  if constexpr (detail::skip_args_count<ParsingTraits>() == 0) {
    detail::println(stream, "Usage: COMMAND [OPTIONS...]");
  } else {
    detail::println(
      stream,
      "Usage: {} COMMAND [OPTIONS...]",
      get_prefix_for_user_messages<ParsingTraits>(argv));
  }

  if constexpr (has_description<RootTraits>) {
    detail::println(stream, "{}", RootTraits::description);
  }

  detail::println(stream, "\nCommands:\n");

  (
    [&]<class T> {
      static constexpr auto nameBuffer = subcommand_name<RootTraits, T>();
      constexpr std::string_view name {nameBuffer};

      using TArgs = typename T::arguments_type;
      if constexpr (has_description<TArgs>) {
        detail::println(stream, "      {:24} {}", name, TArgs::description);
      } else {
        detail::println(stream, "      {}", name);
      }
    }.template operator()<Ts>(),
    ...);

  detail::println(
    stream,
    "\n  {:2}, {:24} show this message",
    *CommonArguments::short_help,
    *CommonArguments::long_help);

  if constexpr (has_version<RootTraits>) {
    detail::println(
      stream, "      {:24} print program version", *CommonArguments::version);
  }

  if constexpr (detail::skip_args_count<ParsingTraits>() == 0) {
    detail::println(
      stream,
      "\nFor more information, run:\n\n  COMMAND {}",
      *CommonArguments::long_help);
  } else {
    detail::println(
      stream,
      "\nFor more information, run:\n\n  {} COMMAND {}",
      get_prefix_for_user_messages<ParsingTraits>(argv),
      *CommonArguments::long_help);
  }
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const help_requested&,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  show_command_usage<Traits, Ts...>(argv, outputStream);
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const version_requested&,
  argv_range auto&&,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  if constexpr (has_version<Traits>) {
    detail::println(outputStream, "{}", Traits::version);
  } else {
    throw std::logic_error(
      "magic_args: somehow got root command version_requested without a "
      "version");
  }
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const missing_required_argument&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  using ParsingTraits = root_command_parsing_traits_t<Traits>;
  detail::print(
    errorStream,
    "{}: You must specify a COMMAND",
    get_prefix_for_user_messages<ParsingTraits>(argv));
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const invalid_argument_value& r,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  using ParsingTraits = root_command_parsing_traits_t<Traits>;
  detail::print(
    errorStream,
    "{}: `{}` is not a valid COMMAND",
    get_prefix_for_user_messages<ParsingTraits>(argv),
    r.mSource.mValue);
}

}// namespace magic_args::detail

#endif
