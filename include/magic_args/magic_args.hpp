// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_MAGIC_ARGS_HPP
#define MAGIC_ARGS_MAGIC_ARGS_HPP
#ifdef MAGIC_ARGS_SINGLE_FILE
namespace magic_args {
constexpr bool is_single_header_file = true;
}
#else
namespace magic_args {
constexpr bool is_single_header_file = false;
}
#include "dump.hpp"
#include "gnu_style_parsing_traits.hpp"
#include "parse.hpp"
#include "powershell_style_parsing_traits.hpp"
#include "verbatim_names.hpp"

#ifndef MAGIC_ARGS_DISABLE_ENUM
#if __has_include(<magic_enum/magic_enum.hpp>)
#include "enum.hpp"
#endif

#endif
#endif

#endif