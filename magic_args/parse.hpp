// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/get_argument_definition.hpp"
#include "detail/parse.hpp"
#include "detail/print_incomplete_parse_reason.hpp"
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
std::expected<T, incomplete_parse_reason_t> parse_silent(
  std::span<std::string_view> args,
  const program_info& help = {}) {
  using namespace detail;

  const auto longHelp
    = std::format("{}{}", Traits::long_arg_prefix, Traits::long_help_arg);
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
    = std::format("{}{}", Traits::long_arg_prefix, Traits::version_arg);

  for (auto&& arg: args) {
    if (arg == "--") {
      break;
    }
    if (arg == longHelp || (arg == shortHelp && !shortHelp.empty())) {
      return std::unexpected {help_requested {}};
    }
    if (arg == versionArg && !help.mVersion.empty()) {
      return std::unexpected {version_requested {}};
    }
  }

  const auto arg0 = std::filesystem::path {args.front()}.stem().string();
  T ret {};
  auto tuple = tie_struct(ret);

  constexpr auto N = count_members<T>();
  std::vector<std::string_view> positionalArgs;

  // Handle options
  std::optional<incomplete_parse_reason_t> failure;
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
      return std::unexpected {failure.value()};
    }
    if (matchedOption) {
      continue;
    }

    if (arg.starts_with(Traits::long_arg_prefix)) {
      return std::unexpected {invalid_argument {
        .mKind = invalid_argument::kind::Option,
        .mSource = {std::string {arg}},
      }};
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
        return std::unexpected {invalid_argument {
          .mKind = invalid_argument::kind::Option,
          .mSource = {std::string {arg}},
        }};
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
      auto result = parse_positional_argument<Traits>(def, positionalArgs);
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
    return std::unexpected {failure.value()};
  }

  if (!positionalArgs.empty()) {
    return std::unexpected {invalid_argument {
      .mKind = invalid_argument::kind::Positional,
      .mSource = {std::string {positionalArgs.front()}},
    }};
  }

  return ret;
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t>
parse_silent(int argc, char** argv, const program_info& help = {}) {
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (auto&& arg: std::span {argv, static_cast<std::size_t>(argc)}) {
    args.emplace_back(arg);
  }
  return parse_silent<T, Traits>(std::span {args}, help);
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse(
  const std::span<std::string_view> args,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_silent<T, Traits>(args, help);
  if (!ret) {
    detail::print_incomplete_parse_reason<T, Traits>(
      help,
      args.empty() ? std::string_view {} : args.front(),
      outputStream,
      errorStream,
      ret.error());
  }
  return ret;
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse(
  const int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_silent<T, Traits>(argc, argv, help);
  if (!ret) {
    detail::print_incomplete_parse_reason<T, Traits>(
      help,
      std::string_view {argc == 0 ? nullptr : argv[0]},
      outputStream,
      errorStream,
      ret.error());
  }
  return ret;
}

}// namespace magic_args::inline public_api
