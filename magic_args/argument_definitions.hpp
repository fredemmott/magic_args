// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

#include <string>
#include <type_traits>

namespace magic_args::inline public_api {

template <class T>
struct optional_positional_argument {
  using value_type = T;
  static constexpr bool is_required = false;
  static constexpr bool is_std_optional = detail::std_optional<T>;
  T mValue {};
  std::string mName;
  std::string mHelp;

  [[nodiscard]] constexpr decltype(auto) value(this auto&& self) noexcept(
    !is_std_optional) {
    if constexpr (is_std_optional) {
      return self.mValue.value();
    } else {
      return self.mValue;
    }
  }

  [[nodiscard]] constexpr bool has_value() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  };

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

  [[nodiscard]] constexpr decltype(auto) value(this auto&& self) noexcept(
    !is_std_optional) {
    if constexpr (is_std_optional) {
      return self.mValue.value();
    } else {
      return self.mValue;
    }
  }
  [[nodiscard]] constexpr bool has_value() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  };

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

  [[nodiscard]] constexpr decltype(auto) value(this auto&& self) noexcept(
    !is_std_optional) {
    if constexpr (is_std_optional) {
      return self.mValue.value();
    } else {
      return self.mValue;
    }
  }
  [[nodiscard]] constexpr bool has_value() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  };

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