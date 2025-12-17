// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_GNU_STYLE_PARSING_TRAITS_HPP
#define MAGIC_ARGS_GNU_STYLE_PARSING_TRAITS_HPP

#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
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

  inline static void normalize_option_name(std::string& name);
  inline static void normalize_positional_argument_name(std::string& name);
};

static_assert(parsing_traits<gnu_style_parsing_traits>);

inline void gnu_style_parsing_traits::normalize_option_name(std::string& name) {
  if (name.starts_with('m')) {
    if (name.size() > 1 && name[1] >= 'A' && name[1] <= 'Z') {
      name = name.substr(1);
    } else if (name.size() > 1 && name[1] == '_') {
      name = name.substr(2);
    }
  }
  if (name.starts_with('_')) {
    name = name.substr(1);
  }
  if (name[0] >= 'A' && name[0] <= 'Z') {
    name[0] -= 'A' - 'a';
  }

  for (std::size_t i = 1; i < name.size(); ++i) {
    if (name[i] == '_') {
      name[i] = '-';
      continue;
    }
    if (name[i] >= 'A' && name[i] <= 'Z') {
      name[i] -= 'A' - 'a';
      name.insert(i, 1, '-');
      ++i;
      continue;
    }
  }
}

inline void gnu_style_parsing_traits::normalize_positional_argument_name(
  std::string& name) {
  normalize_option_name(name);
  for (auto&& c: name) {
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
      continue;
    }
    if (c == '-') {
      c = '_';
      continue;
    }
  }
}

}// namespace magic_args::inline public_api

#endif