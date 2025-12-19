// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_PARSE_SILENT_HPP
#define MAGIC_ARGS_PARSE_SILENT_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "detail/config.hpp"
#include "detail/overloaded.hpp"
#include "detail/parse.hpp"
#include "detail/parsing_traits_for_args.hpp"
#include "detail/static_assert_not_an_enum.hpp"
#include "detail/validation.hpp"
#include "detail/visitors.hpp"
#include "gnu_style_parsing_traits.hpp"
#endif

#include <expected>
#include <filesystem>
#include <ranges>

namespace magic_args::inline public_api {

template <parsing_traits Traits, class T>
std::expected<T, incomplete_parse_reason_t> parse_silent(
  detail::argv_range auto&& argv) {
  using namespace detail;
  using CommonArguments = common_arguments_t<Traits>;

  T ret {};

#ifdef MAGIC_ARGS_DISABLE_ENUM
  std::ignore = visit_all_defined_arguments<Traits>(
    []<class It>(const It&, auto&&) {
      static_assert_not_an_enum<typename It::value_type>();
      return false;
    },
    ret);
#endif

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
    if constexpr (has_version<T>) {
      if (arg == CommonArguments::version) {
        return std::unexpected {version_requested {}};
      }
    }
  }

  const auto arg0 = std::filesystem::path {args.front()}.stem().string();

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

    const auto matchedOption = detail::visit_options<Traits>(
      [&](const auto& def, auto& out) {
        auto result = parse_option<Traits>(def, std::views::drop(args, i));
        if (!result) {
          return false;
        }
        if (!result->has_value()) {
          failure = result->error();
          return true;
        }
        assign_value(out, std::move((*result)->mValue));
        i += (*result)->mConsumed;
        return true;
      },
      ret);

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
          const bool matched = detail::visit_options<Traits>(
            [&](const auto& def, auto& out) {
              if (def.mShortName != std::string_view {&it, 1}) {
                return false;
              }

              return overloaded {
                [](flag& v) {
                  assign_value(v, true);
                  return true;
                },
                [](counted_flag& v) {
                  assign_value(v, counted_flag_value_t::increment());
                  return true;
                },
                [](auto&) { return false; },
              }(out);
            },
            ret);
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
  std::ignore = detail::visit_positional_arguments<Traits>(
    [&](const auto& def, auto& out) {
      auto result = parse_positional_argument<Traits>(def, positionalArgs);
      if (!result) {
        return false;
      }
      if (!result->has_value()) {
        failure = result->error();
        return true;
      }
      assign_value(out, std::move((*result)->mValue));
      positionalArgs.erase(
        positionalArgs.begin(), positionalArgs.begin() + (*result)->mConsumed);
      return false;
    },
    ret);
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

template <class T>
std::expected<T, incomplete_parse_reason_t> parse_silent(
  detail::argv_range auto&& argv) {
  using Traits = detail::parsing_traits_for_args_t<T>;
  return parse_silent<Traits, T>(std::forward<decltype(argv)>(argv));
}

}// namespace magic_args::inline public_api

#endif