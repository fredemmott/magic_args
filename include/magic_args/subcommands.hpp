// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "main_macros.hpp"
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

#define MAGIC_ARGS_SUBCOMMANDS_MAIN(...) \
  MAGIC_ARGS_UTF8_MAIN(argv) { \
    const auto ok = magic_args::invoke_subcommands<__VA_ARGS__>(argv); \
    if (ok) [[likely]] { \
      return *ok; \
    } \
    return magic_args::is_error(ok.error()) ? EXIT_FAILURE : EXIT_SUCCESS; \
  }

namespace magic_args::detail {
template <class...>
struct invoke_multicall_t;

template <
  root_command_traits TTraits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest>
struct invoke_multicall_t<TTraits, First, Rest...> {
  struct Traits : TTraits {
    using parsing_traits
      = multicall_traits<root_command_parsing_traits_t<TTraits>>;
  };
  static_assert(root_command_traits<Traits>);
  static_assert(has_parsing_traits<Traits>);

  template <class TArgv>
  auto operator()(TArgv&& argv) {
    return invoke_subcommands<Traits, First, Rest...>(
      std::forward<TArgv>, argv);
  }
};

template <
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest>
struct invoke_multicall_t<First, Rest...> {
  struct Traits {
    using parsing_traits = multicall_traits<>;
  };
  static_assert(root_command_traits<Traits>);
  static_assert(has_parsing_traits<Traits>);

  template <class TArgv>
  auto operator()(TArgv&& argv) {
    return invoke_subcommands<Traits, First, Rest...>(
      std::forward<TArgv>(argv));
  }
};

}// namespace magic_args::detail

#define MAGIC_ARGS_MULTI_CALL_MAIN(...) \
  MAGIC_ARGS_UTF8_MAIN(argv) { \
    const auto ok = magic_args::detail::invoke_multicall_t<__VA_ARGS__> {}( \
      std::forward<decltype(argv)>(argv)); \
    if (ok) [[likely]] { \
      return *ok; \
    } \
    return magic_args::is_error(ok.error()) ? EXIT_FAILURE : EXIT_SUCCESS; \
  }

#endif