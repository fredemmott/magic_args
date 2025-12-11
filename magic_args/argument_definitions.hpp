// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

#include <type_traits>
#include <string>

// As of 2025-12-11, this is a workaround for:
// - we currently target Ubuntu 24.04 which includes G++13, which
//   doesn't support explicit member parameters
// - using a CRTP helper parent class breaks designated initializers
//   under VS 2022
#define MAGIC_ARGS_DETAIL_DEFINE_VALUE_GETTERS() \
  [[nodiscard]] constexpr auto& value() & noexcept(!is_std_optional) { \
    if constexpr (is_std_optional) { \
      return mValue.value(); \
    } else { \
      return mValue; \
    } \
  } \
\
  [[nodiscard]] constexpr const auto& value() \
    const& noexcept(!is_std_optional) { \
    if constexpr (is_std_optional) { \
      return mValue.value(); \
    } else { \
      return mValue; \
    } \
  } \
\
  [[nodiscard]] constexpr auto&& value() && noexcept(!is_std_optional) { \
    if constexpr (is_std_optional) { \
      return std::move(mValue).value(); \
    } else { \
      return std::move(mValue); \
    } \
  } \
\
  [[nodiscard]] \
  constexpr bool has_value() const noexcept \
    requires is_std_optional \
  { \
    return mValue.has_value(); \
  }

namespace magic_args::inline public_api {

template <class T>
struct optional_positional_argument {
  using value_type = T;
  static constexpr bool is_required = false;
  static constexpr bool is_std_optional = detail::std_optional<T>;
  T mValue {};
  std::string mName;
  std::string mHelp;

  MAGIC_ARGS_DETAIL_DEFINE_VALUE_GETTERS();

  optional_positional_argument& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  constexpr operator T() const noexcept {
    return mValue;
  }
  bool operator==(const optional_positional_argument&) const noexcept = default;
  bool operator==(const T& value) const noexcept {
    return mValue == value;
  }

  constexpr operator bool() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  }

  decltype(auto) operator*() const
    requires is_std_optional
  {
    return *mValue;
  }

  decltype(auto) operator*()
    requires is_std_optional
  {
    return *mValue;
  }
};

template <class T>
struct mandatory_positional_argument {
  static constexpr bool is_required = true;
  static constexpr bool is_std_optional = false;
  using value_type = T;
  static_assert(!detail::std_optional<T>);

  T mValue {};
  std::string mName;
  std::string mHelp;

  MAGIC_ARGS_DETAIL_DEFINE_VALUE_GETTERS();

  mandatory_positional_argument& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }

  bool operator==(const mandatory_positional_argument&) const noexcept
    = default;
  bool operator==(const T& value) const noexcept {
    return mValue == value;
  }
};

template <class T>
struct option final {
  using value_type = T;
  static constexpr bool is_std_optional = detail::std_optional<T>;
  T mValue {};
  std::string mName;
  std::string mHelp;
  std::string mShortName;

  MAGIC_ARGS_DETAIL_DEFINE_VALUE_GETTERS();

  option& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }

  bool operator==(const option&) const noexcept = default;
  bool operator==(const T& value) const noexcept {
    return mValue == value;
  }

  constexpr operator bool() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  }

  decltype(auto) operator*() const
    requires is_std_optional
  {
    return *mValue;
  }

  decltype(auto) operator*()
    requires is_std_optional
  {
    return *mValue;
  }
};

struct flag final {
  using value_type = bool;
  std::string mName;
  std::string mHelp;
  std::string mShortName;
  bool mValue {false};

  flag& operator=(bool value) {
    mValue = value;
    return *this;
  }
  operator bool() const noexcept {
    return mValue;
  }

  bool operator==(const flag&) const noexcept = default;
};

static_assert(basic_option<flag>);
static_assert(basic_option<option<std::string>>);
static_assert(std::is_aggregate_v<option<std::string>>);
}// namespace magic_args::inline public_api