// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/parse.hpp"
#include "detail/print_incomplete_parse_reason.hpp"
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
  using arguments_type = T::arguments_type;

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

  // Skip argv[0] and argv[1], instead of just argv[0]
  struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};

  return subcommand_match<First>(
    parse_silent<typename First::arguments_type, InnerTraits>(
      std::views::drop(argv, 1), help));
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

namespace magic_args::detail {

template <parsing_traits Traits, subcommand... Ts>
void show_subcommand_usage(
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
      // TODO: show help
      detail::println(stream, "      {}", T::name);
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
void print_incomplete_subcommand_parse_reason(
  const help_requested&,
  const program_info& info,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  show_subcommand_usage<Traits, Ts...>(info, argv, outputStream);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_subcommand_parse_reason(
  const version_requested&,
  const program_info& info,
  argv_range auto&&,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  detail::println(outputStream, "{}", info.mVersion);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_subcommand_parse_reason(
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
void print_incomplete_subcommand_parse_reason(
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

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_subcommand_parse_reason(
  const incomplete_subcommand_parse_reason_t& variant,
  const program_info& info,
  argv_range auto&& argv,
  FILE* outputStream,
  FILE* errorStream) {
  std::visit(
    [&]<class R>(R&& it) {
      print_incomplete_subcommand_parse_reason<Traits, Ts...>(
        std::forward<R>(it), info, argv, outputStream, errorStream);
      if constexpr (std::decay_t<R>::is_error) {
        detail::print(errorStream, "\n\n");
        show_subcommand_usage<Traits, Ts...>(info, argv, errorStream);
      }
    },
    variant);
}

}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <parsing_traits Traits, subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_subcommands_silent<Traits, First, Rest...>(argv, help);
  if (!ret) {
    detail::print_incomplete_subcommand_parse_reason<Traits, First, Rest...>(
      ret.error(), help, argv, outputStream, errorStream);
    return ret;
  }

  // Matched a subcommand, but argument/option parsing failed for the subcommand

  std::visit(
    [&argv, &help, outputStream, errorStream]<class T>(T&& it) {
      if (it.has_value()) {
        return;
      }
      // Skip argv[0] and argv[1] for printing

      using Args = std::decay_t<T>::arguments_type;
      struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};

      detail::print_incomplete_parse_reason<Args, InnerTraits>(
        it.error(), help, argv, outputStream, errorStream);
    },
    ret.value());
  return ret;
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<gnu_style_parsing_traits, First, Rest...>(
    argv, help, outputStream, errorStream);
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