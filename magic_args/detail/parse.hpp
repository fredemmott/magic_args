// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include <magic_args/incomplete_parse_reason.hpp>

#include "print.hpp"
#endif

#include <expected>
#include <optional>
#include <span>
#include <sstream>
#include <string>

#ifndef __cpp_lib_expected
static_assert(
  false,
  "Your <expected> header is not compatible with your compiler; if you are "
  "using clang, use libc++");
#endif

namespace magic_args::detail {

inline void from_string_arg(std::string& v, std::string_view arg) {
  v = std::string {arg};
}

template <class T>
void from_string_arg_fallback(T& v, std::string_view arg) {
  // TODO (C++26): we should be able to directly use the string_view
  std::stringstream ss {std::string {arg}};
  ss >> v;
}

enum class option_match_kind {
  NameAndValue,
  NameOnly,
};

template <class Traits, basic_option T>
[[nodiscard]]
std::optional<option_match_kind> option_matches(
  const T& argDef,
  std::string_view arg) {
  const auto name = std::format("{}{}", Traits::long_arg_prefix, argDef.mName);
  if (arg == name) {
    return option_match_kind::NameOnly;
  }
  if (arg.starts_with(name + Traits::value_separator)) {
    return option_match_kind::NameAndValue;
  }
  if constexpr (requires { Traits::short_arg_prefix; }) {
    if (
      (!argDef.mShortName.empty())
      && arg
        == std::format("{}{}", Traits::short_arg_prefix, argDef.mShortName)) {
      return option_match_kind::NameOnly;
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
  = std::optional<std::expected<arg_parse_match<T>, incomplete_parse_reason>>;

template <
  class Traits,
  basic_argument T,
  class V = std::decay_t<typename T::value_type>>
  requires(!basic_option<T>)
arg_parse_result<V> parse_option(
  const T& arg,
  std::span<std::string_view> args) {
  return std::nullopt;
}

template <class T>
void from_string_arg_outer(T& out, std::string_view arg) {
  if constexpr (requires { from_string_arg(out, arg); }) {
    from_string_arg(out, arg);
  } else {
    static_assert(requires(std::stringstream ss, T v) { ss >> v; });
    from_string_arg_fallback(out, arg);
  }
}

template <
  class Traits,
  basic_option T,
  class V = std::decay_t<typename T::value_type>>
arg_parse_result<V> parse_option(
  const T& argDef,
  std::span<std::string_view> args) {
  using enum incomplete_parse_reason;
  using enum option_match_kind;
  const auto match = option_matches<Traits>(argDef, args.front());
  if (!match) {
    return std::nullopt;
  }

  std::size_t consumed = 1;
  std::string_view value;
  switch (match.value()) {
    case NameOnly: {
      if (args.size() == 1) {
        return std::unexpected {MissingArgumentValue};
      }
      value = args[1];
      ++consumed;
      break;
    }
    case NameAndValue: {
      const auto it = args.front().find(Traits::value_separator);
      value = args.front().substr(it + std::size(Traits::value_separator) - 1);
      break;
    }
  }

  V ret {};
  from_string_arg_outer(ret, value);
  return {arg_parse_match {ret, consumed}};
}

template <class Traits>
arg_parse_result<bool> parse_option(
  const flag& arg,
  std::span<std::string_view> args) {
  if (option_matches<Traits>(arg, args.front())) {
    return {arg_parse_match {true, 1}};
  }
  return std::nullopt;
}

template <class Traits, basic_argument T, class V = typename T::value_type>
  requires(!basic_option<T>)
arg_parse_result<V> parse_positional_argument(
  const T& argDef,
  std::string_view arg0,
  std::span<std::string_view> args,
  FILE* errorStream) {
  using namespace detail;

  if (args.empty()) {
    if constexpr (T::is_required) {
      detail::println(
        errorStream, "{}: Missing required argument `{}`", arg0, argDef.mName);
      return std::unexpected {incomplete_parse_reason::MissingRequiredArgument};
    }
    return std::nullopt;
  }
  if constexpr (vector_like<V>) {
    V ret {};
    ret.reserve(args.size());
    for (auto&& arg: args) {
      typename V::value_type v {};
      from_string_arg_outer(v, arg);
      ret.push_back(std::move(v));
    }
    return {arg_parse_match {ret, args.size()}};
  } else {
    V ret {};
    from_string_arg_outer(ret, args.front());
    return arg_parse_match {std::move(ret), 1};
  }
}

template <class Traits, basic_option T, class V = typename T::value_type>
arg_parse_result<V> parse_positional_argument(
  const T& argDef,
  std::string_view arg0,
  std::span<std::string_view> args,
  FILE* errorStream) {
  return std::nullopt;
}

}// namespace magic_args::detail
