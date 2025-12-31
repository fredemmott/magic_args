// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_MAIN_MACROS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_MAIN_MACROS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/main_macros.hpp>
#include "invoke_multicall.hpp"
#include "invoke_subcommands.hpp"
#endif

#define MAGIC_ARGS_SUBCOMMANDS_MAIN(...) \
  MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(__VA_ARGS__) \
  MAGIC_ARGS_UTF8_MAIN(argv) { \
    const auto ok = magic_args::invoke_subcommands<__VA_ARGS__>( \
      std::forward<decltype(argv)>(argv)); \
    if (ok) [[likely]] { \
      return *ok; \
    } \
    return magic_args::is_error(ok.error()) ? EXIT_FAILURE : EXIT_SUCCESS; \
  }

#define MAGIC_ARGS_MULTICALL_MAIN(...) \
  MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(__VA_ARGS__) \
  MAGIC_ARGS_UTF8_MAIN(argv) { \
    const auto ok = magic_args::invoke_multicall<__VA_ARGS__>( \
      std::forward<decltype(argv)>(argv)); \
    if (ok) [[likely]] { \
      return *ok; \
    } \
    return magic_args::is_error(ok.error()) ? EXIT_FAILURE : EXIT_SUCCESS; \
  }

#endif