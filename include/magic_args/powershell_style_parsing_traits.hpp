// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_POWERSHELL_STYLE_PARSING_TRAITS_HPP
#define MAGIC_ARGS_POWERSHELL_STYLE_PARSING_TRAITS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "gnu_style_parsing_traits.hpp"
#endif

#include <string>

namespace magic_args::inline public_api {

struct powershell_style_parsing_traits {
  static constexpr char long_arg_prefix[] = "-";
  static constexpr char short_arg_prefix[] = "-";
  static constexpr char value_separator[] = "=";

  static constexpr char long_help_arg[] = "Help";
  static constexpr char short_help_arg[] = "?";
  static constexpr char version_arg[] = "Version";

  static constexpr bool single_char_short_args = false;

  template <auto Name>
  static consteval auto normalize_option_name() {
    using namespace detail::constexpr_strings;
    return upper_camel_t<remove_prefix_t<Name> {}> {};
  }

  template <auto Name>
  static consteval auto normalize_positional_argument_name() {
    return gnu_style_parsing_traits::normalize_positional_argument_name<Name>();
  }
};
static_assert(parsing_traits<powershell_style_parsing_traits>);

}// namespace magic_args::inline public_api
#endif