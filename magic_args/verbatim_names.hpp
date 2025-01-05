// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "gnu_style_parsing_traits.hpp"
#endif

namespace magic_args::inline public_api {

template <class T>
struct verbatim_names : T {
  static void normalize_option_name(std::string&) {};
  static void normalize_positional_argument_name(std::string&) {};
};

}// namespace magic_args::inline public_api