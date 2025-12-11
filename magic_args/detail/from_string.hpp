// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace magic_args::detail {

template<class T>
struct from_string_t {
  static constexpr void operator()(T& out, std::string_view arg) = delete;
};

template<class T>
concept parseable = requires(T& v, std::string_view arg)
{
  from_string_t<T> {};
  { from_string_t<T> {}(v, arg) } -> std::same_as<void>;
};

template<parseable T>
void from_string(T& out, std::string_view arg) {
  from_string_t<T> {}(out, arg);
}

template <class T>
  requires std::assignable_from<T&, std::string>
struct from_string_t<T> {
  static void operator()(T& out, std::string_view arg) {
    out = std::string {arg};
  }
};

template <class T>
  requires(!std::assignable_from<T&, std::string>)
  && requires(std::stringstream ss, T v) { ss >> v; }
struct from_string_t<T> {
  static constexpr void operator()(T& out, std::string_view arg)
  {
    std::stringstream ss {std::string {arg}};
    ss >> out;
  }
};

template <class T>
  requires requires(T v, std::string_view arg) { from_string(v, arg); }
struct from_string_t<std::optional<T>> {
  static constexpr void operator()(std::optional<T>& out, std::string_view arg) {
    T value {};
    from_string(value, arg);
    out = std::move(value);
  }
};

// ADL version
template <class T>
  requires requires (T& out, std::string_view arg) { from_string_argument(out, arg); }
struct from_string_t<T> {
  static constexpr void operator()(T& out, std::string_view arg) {
    from_string_argument(out, arg);
  }
};

}// namespace magic_args::detail