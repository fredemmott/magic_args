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
#include "detail/encoding.hpp"
#include "dump.hpp"
#include "enum.hpp"
#include "gnu_style_parsing_traits.hpp"
#include "iconv.hpp"
#include "main_macros.hpp"
#include "parse.hpp"
#include "powershell_style_parsing_traits.hpp"
#include "verbatim_names.hpp"
#include "windows.hpp"

#endif
#endif