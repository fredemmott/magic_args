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

template <class T>
struct from_string_t {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) = delete;
};

template <class T>
concept parseable = requires(T& v, std::string_view arg) {
  from_string_t<T> {}(v, arg);
  {
    from_string_t<T> {}(v, arg)
  } -> same_as_ignoring_cvref<std::expected<void, invalid_argument_value>>;
};

template <parseable T>
std::expected<void, invalid_argument_value> from_string(
  T& out,
  std::string_view arg) {
  return from_string_t<T> {}(out, arg);
}

template <class T>
  requires std::assignable_from<T&, std::string>
struct from_string_t<T> {
  static std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) {
    out = std::string {arg};
    return {};
  }
};

template <class T>
  requires(!std::assignable_from<T&, std::string>)
  && requires(std::stringstream ss, T v) { ss >> v; }
struct from_string_t<T> {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) {
    std::stringstream ss {std::string {arg}};
    ss >> out;
    if (ss.fail()) {
      return std::unexpected {invalid_argument_value {}};
    }
    return {};
  }
};

template <class T>
  requires requires(T v, std::string_view arg) { from_string(v, arg); }
struct from_string_t<std::optional<T>> {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    std::optional<T>& out,
    std::string_view arg) {
    T value {};
    const auto inner = from_string(value, arg);
    if (!inner) {
      return inner;
    }
    out = std::move(value);
    return {};
  }
};

template <class T>
concept has_adl_from_string_argument = requires(T& out, std::string_view arg) {
  from_string_argument(out, arg);
  {
    from_string_argument(out, arg)
  } -> same_as_ignoring_cvref<std::expected<void, invalid_argument_value>>;
};

// ADL version
template <has_adl_from_string_argument T>
struct from_string_t<T> {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) {
    return from_string_argument(out, arg);
  }
};

}// namespace magic_args::detail

#endif