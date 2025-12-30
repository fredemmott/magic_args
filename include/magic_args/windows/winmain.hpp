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

template <class T, class Signature>
concept with_argv_encoding_error_main = requires {
  &T::argv_encoding_error_main;
} && matches_signature<&T::argv_encoding_error_main, Signature>;

template <class T>
concept with_argv_encoding_error_win32_main = with_argv_encoding_error_main<
  T,
  int(
    make_utf8_argv_error_t error,
    std::string reason,
    HINSTANCE__* instance,
    int nCmdShow)>;

template <class T>
concept utf8_winmain_handler
  = with_argv_encoding_error_win32_main<T>
  && with_main<
      T,
      int(
        std::remove_cvref_t<decltype(make_utf8_argv())>::value_type argv,
        HINSTANCE__* instance,
        int nCmdShow)>;

// clang-format off
template <class T>
concept winmain_handler = requires {
  typename T::arguments_type;
  &T::main;
  &T::unparsed_arguments_main;
}
&& matches_signature<
  &T::main,
  int(typename T::arguments_type args, HINSTANCE__* instance, int nCmdShow)>
&& matches_signature<
  &T::unparsed_arguments_main,
  int(incomplete_parse_reason_t, std::string output, HINSTANCE__* instance, int nCmdShow)>
&& with_argv_encoding_error_win32_main<T>;
// clang-format on

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
  return T::argv_encoding_error_main(
    std::move(utf8).error(),
    std::move(console).error_str(),
    hInstance,
    nCmdShow);
}

template <winmain_handler T>
struct winmain_impl {
  using arguments_type = T::arguments_type;
  using argv_type = std::remove_cvref_t<decltype(make_utf8_argv())>::value_type;

  static int argv_encoding_error_main(
    make_utf8_argv_error_t&& error,
    std::string&& output,
    HINSTANCE__* hInstance,
    const int nCmdShow) {
    return T::argv_encoding_error_main(
      std::move(error), std::move(output), hInstance, nCmdShow);
  }

  static int
  main(argv_type&& argv, HINSTANCE__* hInstance, const int nCmdShow) {
    capturing_console_output console;
    auto parsed = public_api::parse<arguments_type>(argv, console);
    if (parsed) [[likely]] {
      return T::main(*std::move(parsed), hInstance, nCmdShow);
    }

    auto output = is_error(parsed.error()) ? std::move(console).error_str()
                                           : std::move(console).out_str();

    return T::unparsed_arguments_main(
      std::move(parsed).error(), std::move(output), hInstance, nCmdShow);
  }
};

}// namespace magic_args::detail

#endif
#endif

#define MAGIC_ARGS_UTF8_WINMAIN(HANDLER) \
  int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) { \
    return magic_args::detail::utf8_winmain<HANDLER>(hInstance, nCmdShow); \
  }

#define MAGIC_ARGS_WINMAIN(HANDLER) \
  MAGIC_ARGS_UTF8_WINMAIN(magic_args::detail::winmain_impl<HANDLER>)