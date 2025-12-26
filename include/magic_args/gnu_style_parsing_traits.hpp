// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_GNU_STYLE_PARSING_TRAITS_HPP
#define MAGIC_ARGS_GNU_STYLE_PARSING_TRAITS_HPP

#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "detail/constexpr_strings.hpp"
#endif

namespace magic_args::inline public_api {

struct gnu_style_parsing_traits {
  static constexpr char long_arg_prefix[] = "--";
  static constexpr char short_arg_prefix[] = "-";
  static constexpr char value_separator[] = "=";

  static constexpr char long_help_arg[] = "help";
  static constexpr char short_help_arg[] = "?";
  static constexpr char version_arg[] = "version";

  static constexpr bool single_char_short_args = true;

  template <auto Name>
  static consteval auto normalize_option_name() {
    using namespace detail::constexpr_strings;
    return hyphenate_t<remove_field_prefix_t<Name> {}> {};
  }

  template <auto Name>
  static consteval auto normalize_positional_argument_name() {
    using namespace detail::constexpr_strings;
    return to_upper_t<underscore_t<remove_field_prefix_t<Name> {}> {}> {};
  }
};
static_assert(parsing_traits<gnu_style_parsing_traits>);

}// namespace magic_args::inline public_api

#endif