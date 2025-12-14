// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

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

  inline static void normalize_option_name(std::string& name);
  static void normalize_positional_argument_name(std::string& name) {
    return gnu_style_parsing_traits::normalize_positional_argument_name(name);
  }
};
static_assert(parsing_traits<powershell_style_parsing_traits>);

inline void powershell_style_parsing_traits::normalize_option_name(
  std::string& name) {
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
  if (name[0] >= 'a' && name[0] <= 'z') {
    name[0] += 'A' - 'a';
  }

  for (std::size_t i = 1; i < name.size(); ++i) {
    if (name[i] != '_') {
      continue;
    }
    name.erase(i, 1);
    if (i >= name.size()) {
      break;
    }
    if (name[i] >= 'a' && name[i] <= 'z') {
      name[i] += 'A' - 'a';
      ++i;
      continue;
    }
  }
}

}// namespace magic_args::inline public_api