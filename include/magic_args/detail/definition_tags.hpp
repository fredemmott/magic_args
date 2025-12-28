// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_DEFINITION_TAGS_HPP
#define MAGIC_ARGS_DETAIL_DEFINITION_TAGS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

namespace magic_args::detail {
enum class Behavior {
  Option,
  Flag,
  CountedFlag,
  MandatoryPositionalArgument,
  OptionalPositionalArgument,
};

constexpr bool is_required(const Behavior b) {
  return b == Behavior::MandatoryPositionalArgument;
}

constexpr bool is_positional_argument(const Behavior b) {
  return b == Behavior::MandatoryPositionalArgument
    || b == Behavior::OptionalPositionalArgument;
}

constexpr bool is_option(const Behavior b) {
  return !is_positional_argument(b);
}
}// namespace magic_args::detail

namespace magic_args::detail::definition_tags {
template <Behavior T>
struct tag_t {
  static constexpr auto behavior = T;
};

using option_t = tag_t<Behavior::Option>;
using flag_t = tag_t<Behavior::Flag>;
using counted_flag_t = tag_t<Behavior::CountedFlag>;

template <class T>
concept any_option = same_as_any_of<T, option_t, flag_t, counted_flag_t>;

using mandatory_positional_argument_t
  = tag_t<Behavior::MandatoryPositionalArgument>;
using optional_positional_argument_t
  = tag_t<Behavior::OptionalPositionalArgument>;

template <class T>
concept any_positional_argument = same_as_any_of<
  T,
  mandatory_positional_argument_t,
  optional_positional_argument_t>;

template <class T>
concept any = any_option<T> || any_positional_argument<T>;
}// namespace magic_args::detail::definition_tags

#endif