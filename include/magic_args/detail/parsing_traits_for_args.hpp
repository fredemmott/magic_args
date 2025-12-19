// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PARSING_TRAITS_FOR_ARGS_HPP
#define MAGIC_ARGS_DETAIL_PARSING_TRAITS_FOR_ARGS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "concepts.hpp"
#include "gnu_style_parsing_traits.hpp"
#endif

namespace magic_args::detail {
template <class T>
struct parsing_traits_for_args {
  using type = gnu_style_parsing_traits;
  static_assert(parsing_traits<type>);
};

template <has_parsing_traits T>
struct parsing_traits_for_args<T> {
  using type = typename T::parsing_traits;
  static_assert(parsing_traits<type>);
};

template <class T>
using parsing_traits_for_args_t = parsing_traits_for_args<T>::type;

}// namespace magic_args::detail

#endif
