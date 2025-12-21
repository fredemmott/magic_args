// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_GET_ARGUMENT_DEFINITION_HPP
#define MAGIC_ARGS_DETAIL_GET_ARGUMENT_DEFINITION_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include "reflection.hpp"
#endif

#include <concepts>
#include <string>

namespace magic_args::detail {

template <class T, std::size_t N, parsing_traits Traits>
auto infer_argument_definition() {
  using TValue = std::decay_t<decltype(get<N>(detail::tie_struct(T {})))>;
  static constexpr auto name = []<auto Name> {
    if constexpr (basic_argument<TValue> && !basic_option<TValue>) {
      return Traits::template normalize_positional_argument_name<Name>();
    } else {
      return Traits::template normalize_option_name<Name>();
    }
  }.template operator()<detail::member_name_by_index<T, N>>();

  if constexpr (std::same_as<TValue, bool>) {
    return flag {
      .mName = std::string_view {name},
    };
  } else {
    return option<TValue> {
      .mName = std::string_view {name},
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

#endif