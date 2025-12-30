// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_PRINT_INCOMPLETE_COMMAND_PARSE_REASON_HPP
#define MAGIC_ARGS_SUBCOMMANDS_PRINT_INCOMPLETE_COMMAND_PARSE_REASON_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/console_output.hpp>
#include <magic_args/detail/parse.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::detail {

template <root_command_traits RootTraits, subcommand... Ts>
void show_command_usage(argv_range auto&& argv, text_sink auto& stream) {
  using ParsingTraits = root_command_parsing_traits_t<RootTraits>;
  if constexpr (detail::skip_args_count<ParsingTraits>() == 0) {
    stream.println("Usage: COMMAND [OPTIONS...]");
  } else {
    stream.println(
      "Usage: {} COMMAND [OPTIONS...]",
      get_prefix_for_user_messages<ParsingTraits>(argv));
  }

  if constexpr (has_description<RootTraits>) {
    stream.println("{}", RootTraits::description);
  }

  stream.println("\nCommands:\n");

  (
    [&]<class T> {
      static constexpr auto nameStorage
        = subcommand_name_t<RootTraits, T>::value;
      constexpr std::string_view name {nameStorage};

      using TArgs = typename T::arguments_type;
      if constexpr (has_description<TArgs>) {
        stream.println("      {:24} {}", name, TArgs::description);
      } else {
        stream.println("      {}", name);
      }
    }.template operator()<Ts>(),
    ...);

  if constexpr (parsing_traits_with_short_help<ParsingTraits>) {
    stream.println(
      "\n  {:2}, {:24} show this message",
      std::format(
        "{}{}", ParsingTraits::short_arg_prefix, ParsingTraits::short_help_arg),
      std::format(
        "{}{}", ParsingTraits::long_arg_prefix, ParsingTraits::long_help_arg));
  } else {
    stream.println(

      "\n      {:24} show this message",
      std::format(
        "{}{}", ParsingTraits::long_arg_prefix, ParsingTraits::long_help_arg));
  }

  if constexpr (has_version<RootTraits>) {
    stream.println(
      "      {:24} print program version",
      std::format(
        "{}{}", ParsingTraits::long_arg_prefix, ParsingTraits::version_arg));
  }

  if constexpr (detail::skip_args_count<ParsingTraits>() == 0) {
    stream.println(

      "\nFor more information, run:\n\n  COMMAND {}{}",
      ParsingTraits::long_arg_prefix,
      ParsingTraits::long_help_arg);
  } else {
    stream.println(

      "\nFor more information, run:\n\n  {} COMMAND {}{}",
      get_prefix_for_user_messages<ParsingTraits>(argv),
      ParsingTraits::long_arg_prefix,
      ParsingTraits::long_help_arg);
  }
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const help_requested&,
  argv_range auto&& argv,
  console_output auto& output) {
  show_command_usage<Traits, Ts...>(argv, output.out);
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const version_requested&,
  argv_range auto&&,
  console_output auto& output) {
  if constexpr (has_version<Traits>) {
    output.out.println("{}", Traits::version);
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
  console_output auto& output) {
  using ParsingTraits = root_command_parsing_traits_t<Traits>;
  output.error.print(
    "{}: You must specify a COMMAND",
    get_prefix_for_user_messages<ParsingTraits>(argv));
}

template <root_command_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const invalid_argument_value& r,
  argv_range auto&& argv,
  console_output auto& output) {
  using ParsingTraits = root_command_parsing_traits_t<Traits>;
  output.error.print(
    "{}: `{}` is not a valid COMMAND",
    get_prefix_for_user_messages<ParsingTraits>(argv),
    r.mSource.mValue);
}

}// namespace magic_args::detail

#endif
