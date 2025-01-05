// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/get_argument_definition.hpp"
#include "detail/parse.hpp"
#include "detail/reflection.hpp"
#include "detail/usage.hpp"
#include "detail/validation.hpp"
#include "gnu_style_parsing_traits.hpp"
#include "program_info.hpp"
#endif

#include <expected>
#include <filesystem>
#include <format>
#include <span>

namespace magic_args::inline public_api {

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
  std::span<std::string_view> args,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  using namespace detail;
  std::string helpName("help");
  Traits::normalize_option_name(helpName);
  std::string versionName("version");
  Traits::normalize_option_name(versionName);

  const auto longHelp = std::format("{}{}", Traits::long_arg_prefix, helpName);
  const auto shortHelp = [] {
    if constexpr (requires {
                    Traits::short_help_arg;
                    Traits::short_arg_prefix;
                  }) {
      return std::format(
        "{}{}", Traits::short_arg_prefix, Traits::short_help_arg);
    } else {
      return std::string {};
    }
  }();
  const auto versionArg
    = std::format("{}{}", Traits::long_arg_prefix, versionName);

  for (auto&& arg: args) {
    if (arg == "--") {
      break;
    }
    if (arg == longHelp || (arg == shortHelp && !shortHelp.empty())) {
      show_usage<T, Traits>(outputStream, args.front(), help);
      return std::unexpected {incomplete_parse_reason::HelpRequested};
    }
    if (arg == versionArg && !help.mVersion.empty()) {
      detail::println(outputStream, "{}", help.mVersion);
      return std::unexpected {incomplete_parse_reason::VersionRequested};
    }
  }

  const auto arg0 = std::filesystem::path {args.front()}.stem().string();
  T ret {};
  auto tuple = tie_struct(ret);

  constexpr auto N = count_members<T>();
  std::vector<std::string_view> positionalArgs;

  // Handle options
  std::optional<incomplete_parse_reason> failure;
  for (std::size_t i = 1; i < args.size();) {
    const auto arg = args[i];
    if (arg == "--") {
      std::ranges::copy(
        args.subspan(i + 1), std::back_inserter(positionalArgs));
      break;
    }

    const auto matchedOption
      = [&]<std::size_t... I>(std::index_sequence<I...>) {
          // returns bool: matched option
          return ([&] {
            const auto def = get_argument_definition<T, I, Traits>();
            auto result = parse_option<Traits>(def, args.subspan(i));
            if (!result) {
              return false;
            }
            if (!result->has_value()) {
              failure = result->error();
              return true;
            }
            get<I>(tuple) = std::move((*result)->mValue);
            i += (*result)->mConsumed;
            return true;
          }() || ...);
        }(std::make_index_sequence<N> {});

    if (failure) {
      detail::println(errorStream, "");
      show_usage<T, Traits>(errorStream, args.front(), help);
      return std::unexpected {failure.value()};
    }
    if (matchedOption) {
      continue;
    }

    // Store positional parameters for later
    [&]<std::size_t... I>(std::index_sequence<I...>) {
    }(std::make_index_sequence<N> {});

    if (arg.starts_with(Traits::long_arg_prefix)) {
      detail::print(errorStream, "{}: Unrecognized option: {}\n\n", arg0, arg);
      show_usage<T, Traits>(errorStream, args.front(), help);
      return std::unexpected {incomplete_parse_reason::InvalidArgument};
    }
    if constexpr (requires { Traits::short_arg_prefix; }) {
      // TODO: handle -abc where `a`, `b`, and `c` are all flags

      // The short prefixes have other meanings, e.g.:
      //
      // GNU, Powershell: `-` often means 'stdout'
      // Classic MS: '/' can mean 'root of the filesystem
      if (
        arg.starts_with(Traits::short_arg_prefix)
        && arg != Traits::short_arg_prefix) {
        detail::print(
          errorStream, "{}: Unrecognized option: {}\n\n", arg0, arg);
        show_usage<T, Traits>(errorStream, args.front(), help);
        return std::unexpected {incomplete_parse_reason::InvalidArgument};
      }
    }

    positionalArgs.emplace_back(arg);
    ++i;
  }

  // Handle positional args
  static_assert(only_last_positional_argument_may_have_multiple_values<T>());
  static_assert(
    (first_optional_positional_argument<T>() == -1)
    || (first_optional_positional_argument<T>() >= last_mandatory_positional_argument<T>()));
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    (void)([&] {
      // returns bool: continue
      const auto def = get_argument_definition<T, I, Traits>();
      auto result = parse_positional_argument<Traits>(
        def, arg0, positionalArgs, errorStream);
      if (!result) {
        return true;
      }
      if (!result->has_value()) {
        failure = result->error();
        return false;
      }
      get<I>(tuple) = std::move((*result)->mValue);
      positionalArgs.erase(
        positionalArgs.begin(), positionalArgs.begin() + (*result)->mConsumed);
      return true;
    }() && ...);
  }(std::make_index_sequence<N> {});
  if (failure) {
    detail::println(errorStream, "");
    show_usage<T, Traits>(errorStream, args.front(), help);
    return std::unexpected {failure.value()};
  }

  if (!positionalArgs.empty()) {
    detail::print(
      errorStream,
      "{}: Invalid positional argument: {}\n\n",
      arg0,
      positionalArgs.front());
    show_usage<T, Traits>(errorStream, args.front(), help);
    return std::unexpected {incomplete_parse_reason::InvalidArgument};
  }

  return ret;
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
  int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (auto&& arg: std::span {argv, static_cast<std::size_t>(argc)}) {
    args.emplace_back(arg);
  }
  return parse<T, Traits>(std::span {args}, help, outputStream, errorStream);
}

}// namespace magic_args::inline public_api
