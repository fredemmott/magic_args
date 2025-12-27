// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_DEFINITION_TAGS_HPP
#define MAGIC_ARGS_DETAIL_DEFINITION_TAGS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

namespace magic_args::detail::definition_tags {
struct option_t {};
struct flag_t {};
struct counted_flag_t {};

template <class T>
concept any_option = same_as_any_of<T, option_t, flag_t, counted_flag_t>;

struct mandatory_positional_argument_t {};
struct optional_positional_argument_t {};

template <class T>
concept any_positional_argument = same_as_any_of<
  T,
  mandatory_positional_argument_t,
  optional_positional_argument_t>;

template <class T>
concept any = any_option<T> || any_positional_argument<T>;
}// namespace magic_args::detail::definition_tags

#endif