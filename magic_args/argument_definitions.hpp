// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

#include <string>

namespace magic_args::inline api {

template <class T>
struct optional_positional_argument {
  static constexpr bool is_required = false;
  using value_type = T;
  std::string mName;
  std::string mHelp;
  T mValue {};

  optional_positional_argument& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }
  bool operator==(const optional_positional_argument&) const noexcept = default;
  bool operator==(const T& value) const noexcept {
    return mValue == value;
  }
};

template <class T>
struct mandatory_positional_argument {
  static constexpr bool is_required = true;
  using value_type = T;
  std::string mName;
  std::string mHelp;
  T mValue {};

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
  std::string mName;
  std::string mHelp;
  std::string mShortName;
  T mValue {};

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
}// namespace magic_args::inline api