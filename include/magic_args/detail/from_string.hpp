// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_FROM_STRING_HPP
#define MAGIC_ARGS_DETAIL_FROM_STRING_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/incomplete_parse_reason.hpp>
#include "concepts.hpp"
#endif

#include <expected>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace magic_args::detail {

using from_string_result = std::expected<void, invalid_argument_value>;

template <class T>
struct from_string_t {
  static constexpr from_string_result operator()(T& out, std::string_view arg)
    = delete;
};

template <class T>
concept parseable = requires(T& v, std::string_view arg) {
  from_string_t<T> {}(v, arg);
  { from_string_t<T> {}(v, arg) } -> same_as_ignoring_cvref<from_string_result>;
};

template <class TOut, std::convertible_to<std::string_view> TIn>
  requires parseable<std::remove_cvref_t<TOut>>
from_string_result from_string(TOut&& out, TIn&& in) {
  return from_string_t<std::remove_cvref_t<TOut>> {}(
    std::forward<TOut>(out), std::forward<TIn>(in));
}

template <class T>
  requires std::assignable_from<T&, std::string_view>
struct from_string_t<T> {
  static from_string_result operator()(T& out, std::string_view arg) {
    out = arg;
    return {};
  }
};

template <class T>
  requires(!std::assignable_from<T&, std::string>)
  && requires(std::stringstream ss, T v) { ss >> v; }
struct from_string_t<T> {
  static constexpr from_string_result operator()(T& out, std::string_view arg) {
    std::stringstream ss {std::string {arg}};
    ss >> out;
    if (ss.fail()) {
      return std::unexpected {invalid_argument_value {}};
    }
    return {};
  }
};

template <parseable T>
struct from_string_t<std::optional<T>> {
  static constexpr from_string_result operator()(
    std::optional<T>& out,
    std::string_view arg) {
    T value {};
    const auto inner = from_string_t<T> {}(value, arg);
    if (!inner) {
      return inner;
    }
    out = std::move(value);
    return {};
  }
};

template <class T>
concept has_adl_from_argument_value = requires(T& out, std::string_view arg) {
  from_argument_value(out, arg);
  { from_argument_value(out, arg) } -> std::convertible_to<from_string_result>;
};

// ADL version
template <has_adl_from_argument_value T>
struct from_string_t<T> {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) {
    return from_argument_value(out, arg);
  }
};

}// namespace magic_args::detail

#endif