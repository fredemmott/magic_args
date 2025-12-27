// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_VALIDATION_HPP
#define MAGIC_ARGS_DETAIL_VALIDATION_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/gnu_style_parsing_traits.hpp>
#include "get_argument_definition.hpp"
#include "reflection.hpp"
#endif

namespace magic_args::detail {

template <class T, std::size_t I = 0>
constexpr bool only_last_positional_argument_may_have_multiple_values() {
  // Doesn't matter for this check, but we need some traits for
  // get_argument_definition
  using Traits = gnu_style_parsing_traits;

  constexpr auto N = count_members<T>();
  if constexpr (I == N) {
    return true;
  } else if constexpr (vector_like<
                         decltype(get_argument_definition<T, I, Traits>())>) {
    return I == N - 1;
  } else {
    return only_last_positional_argument_may_have_multiple_values<T, I + 1>();
  }
}

template <
  class T,
  std::ptrdiff_t I = static_cast<std::ptrdiff_t>(count_members<T>()) - 1>
constexpr std::ptrdiff_t last_mandatory_positional_argument() {
  if constexpr (I == -1) {
    return -1;
  } else {
    using Traits = gnu_style_parsing_traits;
    using TArg
      = std::decay_t<decltype(get_argument_definition<T, I, Traits>())>;

    if constexpr (TArg::is_required && TArg::is_positional_argument) {
      return I;
    } else {
      return last_mandatory_positional_argument<T, I - 1>();
    }
  }
};

template <class T, std::size_t I = 0, std::size_t N = count_members<T>()>
constexpr std::ptrdiff_t first_optional_positional_argument() {
  if constexpr (I == N) {
    return -1;
  } else {
    using Traits = gnu_style_parsing_traits;
    using TArg
      = std::decay_t<decltype(get_argument_definition<T, I, Traits>())>;

    if constexpr (TArg::is_positional_argument && !TArg::is_required) {
      return I;
    } else {
      return first_optional_positional_argument<T, I + 1, N>();
    }
  }
};

}// namespace magic_args::detail

#endif