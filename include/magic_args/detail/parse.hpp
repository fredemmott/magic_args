// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PARSE_HPP
#define MAGIC_ARGS_DETAIL_PARSE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include <magic_args/incomplete_parse_reason.hpp>

#include "constexpr_strings.hpp"
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

// This exists because we can't declare `static constexpr` variables in structs
// defined inside functions
template <size_t N>
struct skip_args_count_trait {
  static constexpr std::size_t skip_args_count = N;
};

template <parsing_traits Traits>
constexpr std::size_t skip_args_count() {
  if constexpr (requires {
                  {
                    Traits::skip_args_count
                  } -> std::convertible_to<std::size_t>;
                }) {
    return Traits::skip_args_count;
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
  for (std::size_t i = 1; i < detail::skip_args_count<Traits>(); ++i) {
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
  static constexpr auto long_help = [] {
    static constexpr auto storage = constexpr_strings::
      concat_t<T::long_arg_prefix, T::long_help_arg>::value;
    return std::string_view {storage};
  }();

  static constexpr auto short_help = [] {
    if constexpr (std::string_view {T::short_arg_prefix}.empty()) {
      return std::string_view {};
    } else {
      static constexpr auto storage = constexpr_strings::
        concat_t<T::short_arg_prefix, T::short_help_arg>::value;
      return std::string_view {storage};
    }
  }();

  static constexpr auto version = [] {
    static constexpr auto storage
      = constexpr_strings::concat_t<T::long_arg_prefix, T::version_arg>::value;
    return std::string_view {storage};
  }();
};

template <parsing_traits Traits, static_basic_argument TDef>
std::string provided_argument_name(const std::string_view arg) {
  if constexpr (is_positional_argument(TDef::behavior)) {
    return std::string {TDef::name};
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

[[nodiscard]] inline bool consume(
  std::string_view& sv,
  const std::string_view prefix) {
  if (!sv.starts_with(prefix)) {
    return false;
  }
  sv.remove_prefix(prefix.size());
  return true;
}

struct option_match {
  // MUST be entirely within `std::string_view arg`
  std::string_view mName;
  // unless `nullopt`, MUST be entirely within `std::string_view arg`
  std::optional<std::string_view> mValue;

  constexpr bool has_value() const noexcept {
    return mValue.has_value();
  }
};

template <parsing_traits Traits, static_basic_option TDef>
[[nodiscard]]
std::optional<option_match> option_matches_long(const std::string_view arg) {
  using namespace constexpr_strings;

  static constexpr auto PrefixStorage
    = concat_t<Traits::long_arg_prefix, TDef::name>::value;
  constexpr auto Prefix = std::string_view {PrefixStorage};

  if (!arg.starts_with(Prefix)) {
    return std::nullopt;
  }

  const std::string_view nameSlice {arg.begin(), arg.begin() + Prefix.size()};

  std::string_view tail {arg.begin() + Prefix.size(), arg.end()};

  if (tail.empty()) {
    // `--foo`
    return option_match {nameSlice};
  }

  if (!consume(tail, Traits::value_separator)) {
    // `--foobar` when we want `--foo=`
    return std::nullopt;
  }

  return option_match {nameSlice, tail};
}

template <parsing_traits Traits, static_basic_option TDef>
[[nodiscard]]
std::optional<option_match> option_matches_short(const std::string_view arg) {
  if constexpr (std::string_view {Traits::short_arg_prefix}.empty()) {
    return std::nullopt;
  } else if constexpr (TDef::short_name.empty()) {
    return std::nullopt;
  } else {
    if (!arg.starts_with(Traits::short_arg_prefix)) {
      return std::nullopt;
    }
    const auto tail
      = arg.substr(std::string_view {Traits::short_arg_prefix}.size());
    if (tail != TDef::short_name) {
      return std::nullopt;
    }

    return option_match {arg};
  }
}

template <parsing_traits Traits, static_basic_option TDef>
  requires(TDef::behavior == Behavior::Flag)
std::optional<option_match> option_matches_negated_flag(
  const std::string_view arg) {
  if constexpr (!parsing_traits_with_negated_flags<Traits>) {
    return std::nullopt;
  } else {
    static constexpr auto Expected = constexpr_strings::concat_t<
      Traits::long_arg_prefix,
      Traits::template negated_flag_name<TDef::name>()> {};

    if (arg != Expected) {
      return std::nullopt;
    }

    return option_match {arg};
  }
}

template <parsing_traits Traits, static_basic_option TDef>
[[nodiscard]]
std::optional<option_match> option_matches(const std::string_view arg) {
  if (const auto ret = option_matches_long<Traits, TDef>(arg); ret) {
    return ret;
  }

  if (const auto ret = option_matches_short<Traits, TDef>(arg); ret) {
    return ret;
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
  static_basic_positional_argument TArgDef,
  class V = std::decay_t<typename TArgDef::value_type>>
arg_parse_result<V> parse_option(
  [[maybe_unused]] const typename TArgDef::value_type& arg,
  [[maybe_unused]] const random_access_range_of<std::string_view> auto& args) {
  return std::nullopt;
}

template <parsing_traits Traits, static_basic_argument TDef>
auto map_value_parse_error(
  const random_access_range_of<std::string_view> auto& args,
  const std::string_view value,
  invalid_argument_value e) {
  if (!e.mSource.empty()) {
    throw std::logic_error(
      "argument value parsers should not set error source");
  }
  e.mSource = {
    .mArgvSlice = std::ranges::to<std::vector<std::string>>(args),
    .mName = provided_argument_name<Traits, TDef>(*std::ranges::begin(args)),
    .mValue = std::string {value},
  };
  return std::unexpected {std::move(e)};
}

template <
  parsing_traits Traits,
  static_basic_option TDefinition,
  class V = std::decay_t<typename TDefinition::value_type>>
  requires(TDefinition::behavior == Behavior::Option)
arg_parse_result<V> parse_option(
  const random_access_range_of<std::string_view> auto& args) {
  const auto first = *std::ranges::begin(args);

  const auto match = option_matches<Traits, TDefinition>(first);
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
          .mName = provided_argument_name<Traits, TDefinition>(first),
          .mArgvMember = std::string { first },
        },
      }};
    }
    value = args[1];
    ++consumed;
  }

  V ret {};
  if (const auto converted = from_string(ret, value); !converted) {
    return map_value_parse_error<Traits, TDefinition>(
      std::views::take(args, consumed), value, converted.error());
  }
  return {arg_parse_match {ret, consumed}};
}

template <parsing_traits Traits, static_basic_option TArgDef>
  requires(TArgDef::behavior == Behavior::Flag)
arg_parse_result<bool> parse_option(
  const random_access_range_of<std::string_view> auto& args) {
  if (option_matches<Traits, TArgDef>(*std::ranges::begin(args))) {
    return {arg_parse_match {true, 1}};
  }
  if (option_matches_negated_flag<Traits, TArgDef>(*std::ranges::begin(args))) {
    return {arg_parse_match {false, 1}};
  }
  return std::nullopt;
}

struct counted_flag_value_t {
  enum class kind {
    Increase,
    Assign,
  };
  kind mKind {};
  std::size_t mCount {};

  static constexpr counted_flag_value_t increment() {
    return {kind::Increase, 1};
  }
};

template <parsing_traits Traits, static_basic_option TArgDef>
  requires(TArgDef::behavior == Behavior::CountedFlag)
arg_parse_result<counted_flag_value_t> parse_option(
  const random_access_range_of<std::string_view> auto& args) {
  using enum counted_flag_value_t::kind;
  const auto match = option_matches<Traits, TArgDef>(*std::ranges::begin(args));
  if (!match) {
    return std::nullopt;
  }

  if (!match->has_value()) {
    return {arg_parse_match {counted_flag_value_t {Increase, 1}, 1}};
  }

  std::size_t value {};
  if (const auto ret = from_string(value, *match->mValue); !ret) {
    return map_value_parse_error<Traits, TArgDef>(
      args, *match->mValue, ret.error());
  }
  return {arg_parse_match {counted_flag_value_t {Assign, value}, 1}};
}

template <class TArg, class TValue>
  requires requires(TArg& arg, TValue&& value) {
    arg = std::forward<TValue>(value);
  }
void assign_value(TArg& arg, TValue&& value) {
  arg = std::forward<TValue>(value);
}

inline void assign_value(counted_flag& arg, const counted_flag_value_t& value) {
  using enum counted_flag_value_t::kind;
  switch (value.mKind) {
    case Increase:
      arg.mValue += value.mCount;
      break;
    case Assign:
      arg.mValue = value.mCount;
      break;
  }
}

template <
  parsing_traits Traits,
  static_basic_positional_argument TArgDef,
  class V = typename TArgDef::value_type>
arg_parse_result<V> parse_positional_argument(
  const random_access_range_of<std::string_view> auto& args) {
  using namespace detail;

  if (args.empty()) {
    if constexpr (is_required(TArgDef::behavior)) {
      return std::unexpected {
        missing_required_argument {std::string {TArgDef::name}}};
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
        return map_value_parse_error<Traits, TArgDef>(
          std::views::single(*(std::ranges::begin(args) + i)),
          arg,
          parsed.error());
      }
      ret.push_back(std::move(v));
    }
    return {arg_parse_match {ret, args.size()}};
  } else {
    V ret {};
    if (const auto parsed = from_string(ret, args.front()); !parsed) {
      return map_value_parse_error<Traits, TArgDef>(
        std::views::take(args, 1), args.front(), parsed.error());
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

#endif
