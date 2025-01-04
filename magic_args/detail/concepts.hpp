// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <concepts>
#include <optional>
#include <string>

namespace magic_args::inline public_api {
template <class T>
concept basic_argument = requires(T v) {
  typename T::value_type;
  { v.mName } -> std::convertible_to<std::string>;
  { v.mHelp } -> std::convertible_to<std::string>;
};

template <class T>
concept basic_option = requires(T v) {
  typename T::value_type;
  { v.mName } -> std::convertible_to<std::string>;
  { v.mHelp } -> std::convertible_to<std::string>;
  { v.mShortName } -> std::convertible_to<std::string>;
};
}// namespace magic_args::inline api

namespace magic_args::detail {
template <class T>
concept vector_like = requires { typename T::value_type; }
  && requires(T c, typename T::value_type v) { c.push_back(v); }
  && (!std::same_as<T, std::string>);

template <class T>
concept std_optional = requires { typename T::value_type; }
  && std::same_as<T, std::optional<typename T::value_type>>;

}// namespace magic_args::detail