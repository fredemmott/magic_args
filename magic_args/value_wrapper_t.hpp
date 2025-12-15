// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <utility>

namespace magic_args {
template <class TValue>
class value_wrapper_t {
 public:
  using value_type = TValue;

  value_wrapper_t() = delete;
  constexpr explicit value_wrapper_t(TValue value) : mValue {std::move(value)} {
  }

  template <class Self>
  decltype(auto) value(this Self&& self) noexcept {
    return std::forward<Self>(self).mValue;
  }

  template <class Self>
  constexpr decltype(auto) operator*(this Self&& self) noexcept {
    return std::forward<Self>(self).mValue;
  }

 private:
  TValue mValue;
};
}// namespace magic_args
