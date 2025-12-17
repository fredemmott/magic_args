// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include "reflection.hpp"
#endif

#include <concepts>
#include <string>

namespace magic_args::detail {

template <class T, std::size_t N, parsing_traits Traits>
auto infer_argument_definition() {
  // TODO: put the member name -> thing into the traits
  std::string name {detail::member_name<T, N>};
  using TValue = std::decay_t<decltype(get<N>(detail::tie_struct(T {})))>;
  if constexpr (basic_argument<TValue> && !basic_option<TValue>) {
    Traits::normalize_positional_argument_name(name);
  } else {
    Traits::normalize_option_name(name);
  }

  if constexpr (std::same_as<TValue, bool>) {
    return flag {
      .mName = name,
    };
  } else {
    return option<TValue> {
      .mName = name,
    };
  }
}

template <class T, std::size_t N, parsing_traits Traits>
auto get_argument_definition() {
  using namespace detail;

  auto value = get<N>(tie_struct(T {}));
  using TValue = std::decay_t<decltype(value)>;
  if constexpr (basic_argument<TValue>) {
    if (value.mName.empty()) {
      value.mName = infer_argument_definition<T, N, Traits>().mName;
    }
    return value;
  } else {
    return infer_argument_definition<T, N, Traits>();
  }
}
}// namespace magic_args::detail