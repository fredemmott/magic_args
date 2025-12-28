// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_PARSE_SUBCOMMANDS_SILENT_HPP
#define MAGIC_ARGS_SUBCOMMANDS_PARSE_SUBCOMMANDS_SILENT_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/concepts.hpp>
#include <magic_args/gnu_style_parsing_traits.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::detail {

template <root_command_traits>
struct root_command_parsing_traits {
  using type = gnu_style_parsing_traits;
};

template <root_command_traits T>
  requires parsing_traits<T>
struct root_command_parsing_traits<T> {
  using type = T;
};

template <root_command_traits T>
  requires has_parsing_traits<T>
struct root_command_parsing_traits<T> {
  using type = typename T::parsing_traits;
};

template <root_command_traits T>
using root_command_parsing_traits_t = root_command_parsing_traits<T>::type;

template <parsing_traits Traits>
struct command_from_argument_t {
  static constexpr std::string_view operator()(const std::string_view arg) {
    return arg;
  }
};

template <parsing_traits Traits>
  requires requires(const std::string_view v) {
    {
      Traits::command_from_argument(v)
    } -> same_as_ignoring_cvref<std::string_view>;
  }
struct command_from_argument_t<Traits> {
  static constexpr std::string_view operator()(const std::string_view arg) {
    return Traits::command_from_argument(arg);
  }
};

template <parsing_traits Traits>
std::string_view command_from_argument(const std::string_view arg) {
  return command_from_argument_t<Traits> {}(arg);
}

template <parsing_traits TRootTraits, class T>
struct subcommand_parsing_traits_t : TRootTraits {
  static constexpr auto skip_args_count
    = detail::skip_args_count<TRootTraits>() + 1;
};

template <parsing_traits TRootTraits, has_parsing_traits T>
struct subcommand_parsing_traits_t<TRootTraits, T> : T::parsing_traits {
  static constexpr auto skip_args_count
    = detail::skip_args_count<TRootTraits>() + 1;
};

template <
  root_command_traits RootTraits,
  parsing_traits ParsingTraits,
  subcommand First,
  subcommand... Rest,
  class TExpected>
void parse_subcommands_silent_impl(
  std::optional<TExpected>& result,
  std::string_view command,
  argv_range auto&& argv) {
  if (subcommand_name_t<RootTraits, First> {} != command) {
    if constexpr (sizeof...(Rest) > 0) {
      parse_subcommands_silent_impl<RootTraits, ParsingTraits, Rest...>(
        result, command, argv);
    }
    return;
  }

  // Skip argv[0] and argv[1], instead of just argv[0]
  using SubcommandArgs = typename First::arguments_type;
  using SubcommandTraits
    = subcommand_parsing_traits_t<ParsingTraits, SubcommandArgs>;

  auto subcommandResult = parse_silent<SubcommandTraits, SubcommandArgs>(argv);
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
  root_command_traits RootTraits,
  subcommand First,
  subcommand... Rest,
  class TSuccess
  = std::variant<subcommand_match<First>, subcommand_match<Rest>...>,
  class TIncomplete = incomplete_command_parse_reason_t<First, Rest...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected parse_subcommands_silent(detail::argv_range auto&& argv) {
  using ParsingTraits = detail::root_command_parsing_traits<RootTraits>::type;
  const auto commandIndex = detail::skip_args_count<ParsingTraits>();
  if (argv.size() <= commandIndex) {
    return std::unexpected {missing_required_argument {
      .mSource = {.mName = "COMMAND"},
    }};
  }
  const std::string_view commandArg {
    *(std::ranges::begin(argv) + commandIndex)};
  const std::string_view command {
    detail::command_from_argument<ParsingTraits>(commandArg)};

  using CommonArguments = detail::common_arguments_t<ParsingTraits>;

  if (
    command == CommonArguments::long_help
    || command == CommonArguments::short_help || command == "help") {
    return std::unexpected {help_requested {}};
  }

  if constexpr (detail::has_version<RootTraits>) {
    if (command == CommonArguments::version) {
      return std::unexpected {version_requested {}};
    }
  }

  std::optional<TExpected> result;
  detail::
    parse_subcommands_silent_impl<RootTraits, ParsingTraits, First, Rest...>(
      result, command, argv);
  if (result) [[likely]] {
    return std::move(result).value();
  }

  return std::unexpected {invalid_argument_value {
        .mSource = {
          .mArgvSlice = std::vector { std::string { commandArg } },
          .mName = "COMMAND",
          .mValue = std::string { command },
        },
      }};
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands_silent(detail::argv_range auto&& argv) {
  return parse_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv));
}

}// namespace magic_args::inline public_api

#endif
