// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_WINDOWS_SUBCOMMANDS_WINMAIN_HPP
#define MAGIC_ARGS_WINDOWS_SUBCOMMANDS_WINMAIN_HPP
#ifdef _WIN32

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/subcommands/invoke_subcommands.hpp>
#include "winmain.hpp"
#endif

namespace magic_args::inline public_api {

template <class T>
struct subcommands_winmain_unexpected {};

template <class First, class... Rest>
struct subcommands_winmain_unexpected<
  invocable_subcommands_list<First, Rest...>> {
  using type = with_output<detail::variant_cat_t<
    make_utf8_argv_error_t,
    incomplete_command_parse_reason_t<First, Rest...>>>;
};

template <class TSubcommandsList>
using subcommands_winmain_unexpected_t
  = subcommands_winmain_unexpected<TSubcommandsList>::type;

template <class TSubcommandsList>
using subcommands_winmain_expected_t = std::expected<
  typename TSubcommandsList::return_type,
  subcommands_winmain_unexpected_t<TSubcommandsList>>;

}// namespace magic_args::inline public_api

namespace magic_args::detail {
template <class T>
concept subcommands_winmain_traits = root_command_traits<T> && requires {
  typename T::subcommands;
  &T::main;
} && requires {
  &T::main;
} && matches_signature<&T::main, int(subcommands_winmain_expected_t<typename T::subcommands> result, HINSTANCE__* instance, int nCmdShow)>;

template <class Root>
  requires subcommands_winmain_traits<Root>
struct subcommands_winmain {
  using unexpected_type
    = subcommands_winmain_unexpected_t<typename Root::subcommands>;

  template <class U>
  static auto convert_unexpected(with_output<U>&& what) {
    auto value = std::visit(
      []<class V>(V&& it) {
        return decltype(unexpected_type {}.value) {std::forward<V>(it)};
      },
      std::move(what.value));

    return std::unexpected {unexpected_type {
      std::move(value),
      std::move(what.output),
    }};
  }

  [[nodiscard]]
  static int main(
    utf8_winmain_expected_t&& argv,
    HINSTANCE__* hInstance,
    const int nCmdShow) {
    if (!argv) [[unlikely]] {
      return Root::main(
        convert_unexpected(std::move(argv.error())), hInstance, nCmdShow);
    }

    capturing_console_output console;
    auto ok = invoke_subcommands<Root>(*argv, console);
    if (ok) [[likely]] {
      return Root::main(std::move(ok).value(), hInstance, nCmdShow);
    }

    auto output = is_error(ok.error()) ? std::move(console).error_str()
                                       : std::move(console).out_str();
    auto unex = convert_unexpected(
      with_output {
        std::move(ok).error(),
        std::move(output),
      });

    return Root::main(std::move(unex), hInstance, nCmdShow);
  }
};

}// namespace magic_args::detail

#define MAGIC_ARGS_SUBCOMMANDS_WINMAIN(ROOT) \
  MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(ROOT) \
  MAGIC_ARGS_UTF8_WINMAIN( \
    magic_args::utf8_winmain_expected_t&& args, \
    HINSTANCE hInstance, \
    int nCmdShow) { \
    return magic_args::detail::subcommands_winmain<Root>::main( \
      std::move(args), hInstance, nCmdShow); \
  }

#endif
#endif
