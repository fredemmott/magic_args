// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_PARSE_SILENT_HPP
#define MAGIC_ARGS_PARSE_SILENT_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "detail/config.hpp"
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
    auto tail = arg;
    if (consume(tail, Traits::long_arg_prefix)) {
      if (tail == Traits::long_help_arg) {
        return std::unexpected {help_requested {}};
      }
      if constexpr (has_version<T>) {
        if (tail == Traits::version_arg) {
          return std::unexpected {version_requested {}};
        }
      }
      continue;
    }
    if (consume(tail, Traits::short_arg_prefix)) {
      if (tail == Traits::short_help_arg && !tail.empty()) {
        return std::unexpected {help_requested {}};
      }
      continue;
    }
  }

  const auto arg0 = std::filesystem::path {args.front()}.stem().string();

  std::vector<std::string_view> positionalArgs;
  positionalArgs.reserve(args.size() - skip_args_count<Traits>());

  // Handle options
  for (std::size_t i = skip_args_count<Traits>(); i < args.size();) {
    const auto rest = std::span {args}.subspan(i);
    const auto arg = rest.front();
    if (arg == "--") {
      // Currently support GCC 14 which doesn't have append_range()
      const auto next = rest.subspan(1);
      positionalArgs.insert(positionalArgs.end(), next.begin(), next.end());
      break;
    }

    if (arg.starts_with(Traits::long_arg_prefix)) {
      const auto match = parse_long_option<Traits>(ret, rest);
      if (match) [[likely]] {
        i += match->consumed_argc;
        continue;
      }
      return std::unexpected {match.error()};
    }

    if constexpr (parsing_traits_with_short_args<Traits>) {
      if (arg.starts_with(Traits::short_arg_prefix)) {
        if (arg.size() == std::string_view {Traits::short_arg_prefix}.size()) {
          positionalArgs.emplace_back(arg);
          ++i;
          continue;
        }

        const auto match = parse_short_option<Traits>(ret, rest);
        if (match) {
          i += match->consumed_argc;
          continue;
        }
        return std::unexpected {match.error()};
      }
    }

    positionalArgs.emplace_back(arg);
    ++i;
  }

  static_assert(only_last_positional_argument_may_have_multiple_values<T>());
  static_assert(
    (first_optional_positional_argument<T>() == -1)
    || (first_optional_positional_argument<T>() >= last_mandatory_positional_argument<T>()));

  // Visit them all in order; visiting will consume argv
  std::optional<incomplete_parse_reason_t> failure;
  std::span<const std::string_view> remainingArgs {positionalArgs};
  std::ignore = visit_positional_arguments<Traits>(
    [&]<static_basic_positional_argument TArgDef>(
      const TArgDef&, auto& memberOut) {
      auto& valueOut = project_value(memberOut);

      const auto result
        = parse_positional_argument<Traits, TArgDef>(remainingArgs, valueOut);
      if (!result) [[unlikely]] {
        failure = result.error();
        return true;
      }
      remainingArgs = remainingArgs.subspan(result->consumed_argc);
      return false;
    },
    ret);
  if (failure) [[unlikely]] {
    return std::unexpected {*failure};
  }

  if (remainingArgs.empty()) [[likely]] {
    return ret;
  }

  return std::unexpected {too_many_arguments {
    .mSource = {std::string {remainingArgs.front()}},
  }};
}

template <class T>
std::expected<T, incomplete_parse_reason_t> parse_silent(
  detail::argv_range auto&& argv) {
  using Traits = detail::parsing_traits_for_args_t<T>;
  return parse_silent<Traits, T>(std::forward<decltype(argv)>(argv));
}

}// namespace magic_args::inline public_api

#endif