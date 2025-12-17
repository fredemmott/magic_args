// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include "subcommands/invoke_subcommands.hpp"
#include "subcommands/invoke_subcommands_silent.hpp"
#include "subcommands/is_error.hpp"
#include "subcommands/parse_subcommands.hpp"
#include "subcommands/parse_subcommands_silent.hpp"
#endif

#define MAGIC_ARGS_HAVE_SUBCOMMANDS

namespace magic_args::inline public_api {
template <parsing_traits T = gnu_style_parsing_traits>
struct multicall_traits : T {
  static constexpr std::size_t skip_args_count = 0;

  static std::string_view command_from_argument(std::string_view arg) {
    const auto directorySeparator = arg.find_last_of("/\\");
    if (directorySeparator != std::string_view::npos) {
      arg.remove_prefix(directorySeparator + 1);
    }

    const auto extensionSeparator = arg.find_last_of('.');
    switch (extensionSeparator) {
      case std::string_view::npos:
        return arg;
      case 0:// .foo -> foo
        return arg.substr(1);
      default:
        return arg.substr(0, extensionSeparator);
    }
  }
};

}// namespace magic_args::inline public_api