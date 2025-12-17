// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_TO_FORMATTABLE_HPP
#define MAGIC_ARGS_DETAIL_TO_FORMATTABLE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "argument_definitions.hpp"
#include "concepts.hpp"
#endif

#include <format>
#include <ranges>
#include <vector>

namespace magic_args::detail {
template <class T>
struct to_formattable_t {
  static constexpr auto operator()(T&&) = delete;
};

template <class T>
concept formattable = requires(T&& v) {
  to_formattable_t<T> {};
  { to_formattable_t<T> {}(std::forward<T>(v)) } -> std::formattable<char>;
};

template <formattable T>
auto to_formattable(T&& v) {
  return to_formattable_t<T> {}(std::forward<T>(v));
}

template <std::formattable<char> T>
struct to_formattable_t<T> {
  static constexpr decltype(auto) operator()(T&& v) {
    return std::forward<T>(v);
  }
};

template <class T>
  requires formattable<decltype(std::declval<T>().value())> && requires(T v) {
    { v.has_value() } -> std::same_as<bool>;
    { v.value() } -> formattable;
  }
struct to_formattable_t<T> {
  static std::string operator()(const T& v) {
    if (v.has_value()) {
      return std::format(
        "{}", to_formattable_t<decltype(v.value())> {}(v.value()));
    }
    return "[nullopt]";
  }
};

template <class T>
  requires requires(T v) {
    { v.value() } -> formattable;
  } && (!requires(T v) { v.has_value(); })
struct to_formattable_t<T> {
  static constexpr auto operator()(const T& v) {
    return to_formattable_t<decltype(v.value())> {}(v.value());
  }
};

template <>
struct to_formattable_t<const flag&> {
  static constexpr auto operator()(const flag& v) {
    return v.mValue ? "true" : "false";
  }
};

template <>
struct to_formattable_t<const counted_flag&> {
  static constexpr auto operator()(const counted_flag& v) {
    return v.mCount;
  }
};

// For Ubuntu 24.04 (GCC 13)
#ifndef __cpp_lib_format_ranges
template <std::ranges::range R>
  requires requires(R r) {
    { *std::ranges::begin(r) } -> formattable;
  } && (!std::same_as<char, std::ranges::range_value_t<R>>)
struct to_formattable_t<R> {
  static constexpr auto operator()(R&& range) {
    // GCC 13 is too old for a lot of the modern ranges stuff, so the good
    // old fashioned way is by far the most readable
    bool first = true;
    std::string out {"["};
    for (auto&& item: range) {
      if (std::exchange(first, false)) {
        out.append(std::format("\"{}\"", to_formattable(item)));
      } else {
        out.append(std::format(", \"{}\"", to_formattable(item)));
      }
    }
    out.append("]");

    return out;
  }
};
#endif

// ADL for `formattable_argument_value()`
//
// Use `requires requires` as G++13 (Ubuntu 24.04) is not happy with the
// trailing-return-type SFINAE trick for undefined ADL
template <class T>
  requires requires(T v) {
    { formattable_argument_value(v) } -> std::formattable<char>;
  }
struct to_formattable_t<T> {
  static constexpr auto operator()(T&& v) {
    return formattable_argument_value(v);
  }
};

}// namespace magic_args::detail

#endif