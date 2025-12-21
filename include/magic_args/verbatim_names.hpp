// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_VERBATIM_NAMES_HPP
#define MAGIC_ARGS_VERBATIM_NAMES_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "gnu_style_parsing_traits.hpp"
#endif

namespace magic_args::inline public_api {

template <parsing_traits T = gnu_style_parsing_traits>
struct verbatim_names : T {
  template <auto Name>
  static consteval auto normalize_option_name() {
    return Name;
  }
  template <auto Name>
  static consteval auto normalize_positional_argument_name() {
    return Name;
  }
};

}// namespace magic_args::inline public_api
#endif
