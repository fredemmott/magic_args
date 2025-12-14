// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include <magic_args/incomplete_parse_reason.hpp>

#include "from_string.hpp"
#endif

#include <algorithm>
#include <expected>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>

#ifndef __cpp_lib_expected
static_assert(
  false,
  "Your <expected> header is not compatible with your compiler; if you are "
  "using clang, use libc++");
#endif

namespace magic_args::detail {

template <size_t N>
struct prefix_args_count_trait {
  static constexpr std::size_t prefix_args_count = N;
};

template <parsing_traits Traits>
constexpr std::size_t prefix_args_count() {
  if constexpr (requires {
                  {
                    Traits::prefix_args_count
                  } -> std::convertible_to<std::size_t>;
                }) {
    return Traits::prefix_args_count;
  } else {
    // By default, skip argv[0]
    return 1;
  }
}

/** Usually just argv[0], but may be something else,
 *
 * e.g. the concatenation of both argv[0] and argv[1] if using subcommands.
 *
 * **DO NOT USE THIS TO LAUNCH SUBPROCESSES** - it does not escape the strings.
 */
template <parsing_traits Traits>
std::string get_prefix_for_user_messages(argv_range auto&& argv) {
  auto prefix
    = std::filesystem::path {*std::ranges::begin(argv)}.stem().string();
  for (std::size_t i = 1; i < detail::prefix_args_count<Traits>(); ++i) {
    const auto it = std::ranges::begin(argv) + i;
    if (it >= std::ranges::end(argv)) {
      break;
    }
    prefix = std::format("{} {}", prefix, *it);
  }
  return prefix;
}

template <parsing_traits T>
struct common_arguments_t {
 private:
  template <std::size_t N, std::size_t M>
  struct concat_t {
    static constexpr std::size_t TotalSize = N + M - 2;
    concat_t() = delete;
    consteval concat_t(const char (&lhs)[N], const char (&rhs)[M]) {
      std::ranges::copy(lhs, lhs + N - 1, mBuf);
      std::ranges::copy(rhs, rhs + M - 1, mBuf + N - 1);
    }

    constexpr bool operator==(const std::string_view& other) const noexcept {
      return other == std::string_view {mBuf, TotalSize};
    }

    constexpr operator std::string_view() const noexcept {
      return std::string_view {mBuf, TotalSize};
    }

    constexpr std::string_view operator*() const noexcept {
      return *this;
    }

   private:
    char mBuf[TotalSize] {};
  };
  template <std::size_t N, std::size_t M>
  static consteval auto concat(const char (&lhs)[N], const char (&rhs)[M]) {
    return concat_t<N, M> {lhs, rhs};
  }

 public:
  static constexpr auto long_help
    = concat(T::long_arg_prefix, T::long_help_arg);
  static constexpr auto short_help
    = concat(T::short_arg_prefix, T::short_help_arg);
  static constexpr auto version = concat(T::long_arg_prefix, T::version_arg);
};

template <parsing_traits Traits, basic_argument T>
std::string provided_argument_name(
  const T& argDef,
  const std::string_view arg) {
  if constexpr (!basic_option<T>) {
    return argDef.mName;
  } else {
    const auto index = arg.find(Traits::value_separator);
    if (index == std::string_view::npos) {
      return std::string {arg};
    }
    return std::string {arg.substr(0, index)};
  }
}

[[nodiscard]] inline std::string_view consume(
  std::string_view& sv,
  const std::size_t size) {
  if (size > sv.size()) [[unlikely]] {
    throw std::out_of_range {std::format(
      "Asked to consume {} bytes, but only {} available", size, sv.size())};
  }
  const auto begin = sv.begin();
  const auto end = sv.begin() + size;
  sv.remove_prefix(size);
  return {begin, end};
};

struct option_match {
  // MUST be entirely within `std::string_view arg`
  std::string_view mName;
  // unless `nullopt`, MUST be entirely within `std::string_view arg`
  std::optional<std::string_view> mValue;

  constexpr bool has_value() const noexcept {
    return mValue.has_value();
  }
};

template <parsing_traits Traits, basic_option T>
[[nodiscard]]
std::optional<option_match> option_matches_long(
  const T& argDef,
  const std::string_view arg) {
  constexpr std::string_view Prefix {Traits::long_arg_prefix};
  constexpr std::string_view Separator {Traits::value_separator};

  if (!arg.starts_with(Prefix)) {
    return std::nullopt;
  }
  std::string_view tail {arg.begin() + Prefix.size(), arg.end()};

  if (!tail.starts_with(argDef.mName)) {
    return std::nullopt;
  }

  option_match ret {
    .mName = consume(tail, argDef.mName.size()),
    .mValue = {},
  };

  if (tail.empty()) {
    // `--foo`
    return ret;
  }

  if (!tail.starts_with(Separator)) {
    // `--foobar` when we want `--foo=`
    return std::nullopt;
  }

  // `--foo=` or `--foo=bar`
  tail.remove_prefix(Separator.size());
  ret.mValue = tail;

  return ret;
}

template <parsing_traits Traits, basic_option T>
[[nodiscard]]
std::optional<option_match> option_matches_short(
  const T& argDef,
  const std::string_view arg) {
  if (argDef.mShortName.empty()) {
    return std::nullopt;
  }

  constexpr std::string_view Prefix = Traits::short_arg_prefix;
  if (!arg.starts_with(Prefix)) {
    return std::nullopt;
  }

  const std::string_view tail {arg.begin() + Prefix.size(), arg.end()};
  if (tail != argDef.mShortName) {
    return std::nullopt;
  }

  return option_match {.mName = tail, .mValue = {}};
}

template <parsing_traits Traits, basic_option T>
[[nodiscard]]
std::optional<option_match> option_matches(
  const T& argDef,
  std::string_view arg) {
  if (const auto ret = option_matches_long<Traits>(argDef, arg); ret) {
    return ret;
  }

  if constexpr (requires { Traits::short_arg_prefix; }) {
    if (const auto ret = option_matches_short<Traits>(argDef, arg); ret) {
      return ret;
    }
  }

  return std::nullopt;
}

template <class T>
struct arg_parse_match {
  T mValue;
  std::size_t mConsumed;
};

template <class T>
using arg_parse_result
  = std::optional<std::expected<arg_parse_match<T>, incomplete_parse_reason_t>>;

template <
  parsing_traits Traits,
  basic_argument T,
  class V = std::decay_t<typename T::value_type>>
  requires(!basic_option<T>)
arg_parse_result<V> parse_option(
  [[maybe_unused]] const T& arg,
  [[maybe_unused]] const random_access_range_of<std::string_view> auto& args) {
  return std::nullopt;
}

template <parsing_traits Traits, basic_argument T>
auto map_value_parse_error(
  const random_access_range_of<std::string_view> auto& args,
  const T& argDef,
  const std::string_view value,
  invalid_argument_value e) {
  if (!e.mSource.empty()) {
    throw std::logic_error(
      "argument value parsers should not set error source");
  }
  e.mSource = {
    .mArgvSlice = std::ranges::to<std::vector<std::string>>(args),
    .mName = provided_argument_name<Traits>(argDef, *std::ranges::begin(args)),
    .mValue = std::string {value},
  };
  return std::unexpected {std::move(e)};
}

template <
  parsing_traits Traits,
  basic_option T,
  class V = std::decay_t<typename T::value_type>>
arg_parse_result<V> parse_option(
  const T& argDef,
  const random_access_range_of<std::string_view> auto& args) {
  const auto first = *std::ranges::begin(args);
  const auto match = option_matches<Traits>(argDef, first);
  if (!match) {
    return std::nullopt;
  }

  std::size_t consumed = 1;
  std::string_view value;
  if (match->has_value()) {
    value = *match->mValue;
  } else {
    if (args.size() == 1) {
      return std::unexpected {missing_argument_value {
        .mSource = {
          .mName = provided_argument_name<Traits>(argDef, first),
          .mArgvMember = std::string { first },
        },
      }};
    }
    value = args[1];
    ++consumed;
  }

  V ret {};
  if (const auto converted = from_string(ret, value); !converted) {
    return map_value_parse_error<Traits>(
      std::views::take(args, consumed), argDef, value, converted.error());
  }
  return {arg_parse_match {ret, consumed}};
}

template <parsing_traits Traits>
arg_parse_result<bool> parse_option(
  const flag& arg,
  const random_access_range_of<std::string_view> auto& args) {
  if (option_matches<Traits>(arg, *std::ranges::begin(args))) {
    return {arg_parse_match {true, 1}};
  }
  return std::nullopt;
}

template <
  parsing_traits Traits,
  basic_argument T,
  class V = typename T::value_type>
  requires(!basic_option<T>)
arg_parse_result<V> parse_positional_argument(
  const T& argDef,
  const random_access_range_of<std::string_view> auto& args) {
  using namespace detail;

  if (args.empty()) {
    if constexpr (T::is_required) {
      return std::unexpected {missing_required_argument {argDef.mName}};
    } else {
      return std::nullopt;
    }
  }

  if constexpr (vector_like<V>) {
    V ret {};
    ret.reserve(args.size());
    // As of 2025-12-12, Apple Clang on Github Actions does not support
    // `std::views::enumerate`
    for (std::size_t i = 0; i < args.size(); ++i) {
      const auto& arg = args[i];
      typename V::value_type v {};
      if (const auto parsed = from_string(v, arg); !parsed) {
        return map_value_parse_error<Traits>(
          std::views::single(*(std::ranges::begin(args) + i)),
          argDef,
          arg,
          parsed.error());
      }
      ret.push_back(std::move(v));
    }
    return {arg_parse_match {ret, args.size()}};
  } else {
    V ret {};
    if (const auto parsed = from_string(ret, args.front()); !parsed) {
      return map_value_parse_error<Traits>(
        std::views::take(args, 1), argDef, args.front(), parsed.error());
    }
    return arg_parse_match {std::move(ret), 1};
  }
}

template <
  parsing_traits Traits,
  basic_option T,
  class V = typename T::value_type>
arg_parse_result<V> parse_positional_argument(
  [[maybe_unused]] const T& argDef,
  [[maybe_unused]] std::span<std::string_view> args) {
  return std::nullopt;
}

}// namespace magic_args::detail
