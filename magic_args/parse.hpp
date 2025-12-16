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
#include <ranges>

namespace magic_args::inline public_api {

template <class T, parsing_traits Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  using namespace detail;
  using CommonArguments = common_arguments_t<Traits>;

  const std::vector<std::string_view> args {
    std::ranges::begin(argv), std::ranges::end(argv)};

  for (auto&& arg: args) {
    if (arg == "--") {
      break;
    }
    if (
      arg == CommonArguments::long_help || arg == CommonArguments::short_help) {
      return std::unexpected {help_requested {}};
    }
    if (arg == CommonArguments::version && !help.mVersion.empty()) {
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
  for (std::size_t i = skip_args_count<Traits>(); i < args.size();) {
    const auto arg = args[i];
    if (arg == "--") {
      std::ranges::copy(
        std::views::drop(args, i + 1), std::back_inserter(positionalArgs));
      break;
    }

    const auto matchedOption
      = [&]<std::size_t... I>(std::index_sequence<I...>) {
          // returns bool: matched option
          return ([&] {
            const auto def = get_argument_definition<T, I, Traits>();
            auto result = parse_option<Traits>(def, std::views::drop(args, i));
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

    if constexpr (Traits::single_char_short_args) {
      if (
        arg.starts_with(Traits::short_arg_prefix)
        && arg != Traits::short_arg_prefix) {
        const auto flags
          = arg.substr(std::string_view {Traits::short_arg_prefix}.size());
        for (const char it: flags) {
          const bool matched =
            [&]<std::size_t... I>(std::index_sequence<I...>) {
              return ([&]() {
                const auto& def = get_argument_definition<T, I, Traits>();
                if constexpr (!detail::
                                same_as_ignoring_cvref<decltype(def), flag>) {
                  std::ignore = def;
                  return false;
                } else if (
                  def.mShortName.size() == 1 && def.mShortName.front() == it) {
                  get<I>(tuple) = true;
                  return true;
                } else {
                  return false;
                }
              }() || ...);
            }(std::make_index_sequence<N> {});
          if (!matched) {
            return std::unexpected {invalid_argument {
              .mKind = invalid_argument::kind::Option,
              .mSource = {std::string {arg}},
            }};
          }
        }
        ++i;
        continue;
        }
    }

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

template <class T, parsing_traits Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse_silent(
  const int argc,
  const char* const* argv,
  const program_info& help = {}) {
  return parse_silent<T, Traits>(
    std::views::counted(argv, static_cast<std::size_t>(argc)), help);
}

template <class T, parsing_traits Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse(
  detail::argv_range auto&& argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret
    = parse_silent<T, Traits>(std::forward<decltype(argv)>(argv), help);
  if (!ret) {
    detail::print_incomplete_parse_reason<T, Traits>(
      ret.error(), help, argv, outputStream, errorStream);
  }
  return ret;
}

template <class T, parsing_traits Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason_t> parse(
  const int argc,
  const char* const* argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse<T, Traits>(
    std::views::counted(argv, argc), help, outputStream, errorStream);
}

}// namespace magic_args::inline public_api
