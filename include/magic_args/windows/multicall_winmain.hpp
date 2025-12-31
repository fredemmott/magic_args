// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_MULTICALL_WINMAIN_HPP
#define MAGIC_ARGS_MULTICALL_WINMAIN_HPP
#ifdef _WIN32

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/subcommands/invoke_multicall.hpp>
#include "subcommands_winmain.hpp"
#endif

namespace magic_args::detail {

template <root_command_traits Root>
  requires requires { typename Root::subcommands; }
auto multicall_winmain(
  utf8_winmain_expected_t&& argv,
  HINSTANCE__* instance,
  int nCmdShow) {
  struct NextRoot : Root {
    using parsing_traits
      = multicall_traits<root_command_parsing_traits_t<Root>>;
  };
  return subcommands_winmain<NextRoot>::main(
    std::move(argv), instance, nCmdShow);
}

}// namespace magic_args::detail

#define MAGIC_ARGS_MULTICALL_WINMAIN(ROOT) \
  MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(ROOT) \
  MAGIC_ARGS_UTF8_WINMAIN( \
    magic_args::utf8_winmain_expected_t&& args, \
    HINSTANCE hInstance, \
    int nCmdShow) { \
    return magic_args::detail::multicall_winmain<ROOT>( \
      std::move(args), hInstance, nCmdShow); \
  }

#endif
#endif