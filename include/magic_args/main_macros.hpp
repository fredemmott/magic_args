// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#if defined(MAGIC_ARGS_ENABLE_MAIN_MACROS) || !defined(MAGIC_ARGS_SINGLE_FILE)
#ifndef MAGIC_ARGS_MAIN_MACROS_HPP
#define MAGIC_ARGS_MAIN_MACROS_HPP

#ifdef MAGIC_ARGS_SINGLE_FILE

#if __has_include(<Windows.h>) && !defined(MAGIC_ARGS_DISABLE_IMPLICIT_WINDOWS_EXTENSIONS)
#undef MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#elif __has_include(<iconv.h>) && !defined(MAGIC_ARGS_DISABLE_IMPLICIT_ICONV)
#undef MAGIC_ARGS_ENABLE_ICONV_EXTENSIONS
#define MAGIC_ARGS_ENABLE_ICONV_EXTENSIONS
#endif

#include "magic_args.hpp"

#else

#include "magic_args.hpp"
#if __has_include(<Windows.h>) && !defined(MAGIC_ARGS_DISABLE_IMPLICIT_WINDOWS_EXTENSIONS)
#include "windows.hpp"
#elif __has_include(<iconv.h>)

#if defined(MAGIC_ARGS_ENABLE_IMPLICIT_ICONV_EXTENSIONS_WARNING)
#warning "UTF-8 conversion is not available; u
#else
#include "iconv.hpp"
#endif

#endif

#endif

// region MAGIC_ARGS_UTF8_MAIN
namespace magic_args::detail {
template <class TChar>
using utf8_argv_t = decltype(public_api::make_utf8_argv(
  0,
  static_cast<const TChar* const*>(nullptr)))::value_type;

template <auto TImpl, class TChar>
int utf8_main(int argc, const TChar* const* argv) {
  auto utf8 = public_api::make_utf8_argv(argc, argv);
  // TODO: error handling
  return std::invoke(TImpl, *std::move(utf8));
}

template <class T>
struct function_argument_type_t {
  constexpr explicit function_argument_type_t(int (*)(T)) {
  }
  using type = std::remove_cvref_t<T>;
};

template <auto TImpl, class TArgv>
int parsed_main(TArgv&& argv) {
  using TArg = decltype(function_argument_type_t {TImpl})::type;
  static_assert(std::same_as<std::invoke_result_t<decltype(TImpl), TArg>, int>);
  auto parsed = magic_args::parse<TArg>(std::forward<TArgv>(argv));
  // TODO: error handling
  return std::invoke(TImpl, *std::move(parsed));
}
}// namespace magic_args::detail

#ifdef _WIN32
#define MAGIC_ARGS_UTF8_MAIN(ARGV_NAME) \
  static int magic_args_utf8_main( \
    magic_args::detail::utf8_argv_t<wchar_t> ARGV_NAME); \
  int wmain(const int argc, const wchar_t* const* argv) { \
    return magic_args::detail::utf8_main<&magic_args_utf8_main>(argc, argv); \
  } \
  int magic_args_utf8_main(magic_args::detail::utf8_argv_t<wchar_t> ARGV_NAME)
#else
#define MAGIC_ARGS_UTF8_MAIN(ARGV_NAME) \
  static int magic_args_utf8_main( \
    magic_args::detail::utf8_argv_t<char> ARGV_NAME); \
  int main(const int argc, const char* const* argv) { \
    return magic_args::detail::utf8_main<&magic_args_utf8_main>(argc, argv); \
  } \
  int magic_args_utf8_main(magic_args::detail::utf8_argv_t<char> ARGV_NAME)
#endif

#define MAGIC_ARGS_MAIN(ARGS_TYPE_AND_NAME) \
  static int magic_args_parsed_main(ARGS_TYPE_AND_NAME); \
  MAGIC_ARGS_UTF8_MAIN(argv) { \
    return magic_args::detail::parsed_main<&magic_args_parsed_main>(argv); \
  } \
  int magic_args_parsed_main(ARGS_TYPE_AND_NAME)

#endif
#endif