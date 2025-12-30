// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_WINDOWS_WINMAIN_HPP
#define MAGIC_ARGS_WINDOWS_WINMAIN_HPP
#ifdef _WIN32

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/main_macros.hpp>
#include "encoding.hpp"
#include "win32_api.hpp"
#endif

#include <concepts>

namespace magic_args::inline public_api {

template <class F, class Signature>
struct matches_signature_t : std::false_type {};
template <class F, class Ret, class... Args>
struct matches_signature_t<F, Ret(Args...)> {
  static constexpr bool value = std::is_invocable_r_v<Ret, F, Args...>;
};
template <auto F, class Signature>
concept matches_signature = matches_signature_t<decltype(F), Signature>::value;

template <class T, class Signature>
concept with_main
  = requires { &T::main; } && matches_signature<&T::main, Signature>;

template <class T>
struct with_output {
  T value {};
  std::string output;
};
using utf8_winmain_unexpected_t = with_output<make_utf8_argv_error_t>;
using utf8_winmain_expected_t = std::expected<
  std::remove_cvref_t<decltype(make_utf8_argv())>::value_type,
  utf8_winmain_unexpected_t>;

constexpr bool is_error(const utf8_winmain_unexpected_t&) noexcept {
  return true;
}

template <class T>
concept utf8_winmain_handler = with_main<
  T,
  int(utf8_winmain_expected_t argv, HINSTANCE__* instance, int nCmdShow)>;

template <class T, class U>
struct variant_cat {};

template <class... Ts, class... Us>
struct variant_cat<std::variant<Ts...>, std::variant<Us...>> {
  using type = std::variant<Ts..., Us...>;
};

template <class... Ts>
using variant_cat_t = variant_cat<Ts...>::type;

using winmain_unexpected_t = with_output<
  variant_cat_t<make_utf8_argv_error_t, incomplete_parse_reason_t>>;
template <class T>
using winmain_expected_t = std::expected<T, winmain_unexpected_t>;

inline bool is_error(const winmain_unexpected_t& unexpected) noexcept {
  return std::visit(
    detail::overloaded {
      []<incomplete_parse_reason T>(const T&) { return T::is_error; },
      [](const auto&) { return true; }},
    unexpected.value);
}

template <auto T>
concept winmain_handler = matches_signature<
  T,
  int(
    winmain_expected_t<typename std::remove_cvref_t<
      detail::function_argument_type_t<T, 0>>::value_type> args,
    HINSTANCE__* instance,
    int nCmdShow)>;

}// namespace magic_args::inline public_api

namespace magic_args::detail {

template <utf8_winmain_handler T>
[[nodiscard]]
int utf8_winmain(HINSTANCE__* hInstance, int nCmdShow) {
  auto utf8 = make_utf8_argv();
  if (utf8) [[likely]] {
    return T::main(*std::move(utf8), hInstance, nCmdShow);
  }

  capturing_console_output console;
  print_utf8_error(utf8.error(), console.error);
  return T::main(
    std::unexpected {utf8_winmain_unexpected_t {
      std::move(utf8).error(),
      std::move(console).error_str(),
    }},
    hInstance,
    nCmdShow);
}

template <auto TMain>
  requires winmain_handler<TMain>
struct winmain_impl {
  using arguments_type
    = std::remove_cvref_t<function_argument_type_t<TMain, 0>>::value_type;
  using main_arg_t = winmain_expected_t<arguments_type>;

  template <class U>
  static auto convert_unexpected(U&& what) {
    return std::visit(
      []<class V>(V&& it) {
        return decltype(winmain_unexpected_t {}.value) {std::forward<V>(it)};
      },
      std::forward<U>(what));
  }

  static int main(
    utf8_winmain_expected_t&& argv,
    HINSTANCE__* hInstance,
    const int nCmdShow) {
    if (!argv) [[unlikely]] {
      auto unex = std::unexpected {winmain_unexpected_t {
        convert_unexpected(std::move(argv.error().value)),
        std::move(argv.error().output),
      }};
      return TMain(std::move(unex), hInstance, nCmdShow);
    }

    capturing_console_output console;
    auto parsed = public_api::parse<arguments_type>(*argv, console);
    if (parsed) [[likely]] {
      return TMain(*std::move(parsed), hInstance, nCmdShow);
    }

    auto output = is_error(parsed.error()) ? std::move(console).error_str()
                                           : std::move(console).out_str();

    auto unex = std::unexpected {winmain_unexpected_t {
      convert_unexpected(std::move(parsed).error()),
      std::move(output),
    }};
    return TMain(std::move(unex), hInstance, nCmdShow);
  }
};

}// namespace magic_args::detail

#endif
#endif

#define MAGIC_ARGS_UTF8_WINMAIN(HANDLER) \
  int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) { \
    return magic_args::detail::utf8_winmain<HANDLER>(hInstance, nCmdShow); \
  }

#define MAGIC_ARGS_WINMAIN(...) \
  static int magic_args_winmain(__VA_ARGS__); \
  MAGIC_ARGS_UTF8_WINMAIN( \
    magic_args::detail::winmain_impl<&magic_args_winmain>) \
  int magic_args_winmain(__VA_ARGS__)