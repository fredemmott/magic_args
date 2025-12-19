// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_MAIN_MACROS_HPP
#define MAGIC_ARGS_MAIN_MACROS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "iconv.hpp"
#include "parse.hpp"
#include "windows.hpp"
#endif

namespace magic_args::detail {
template <class TChar>
using utf8_argv_t = decltype(public_api::make_utf8_argv(
  0,
  static_cast<const TChar* const*>(nullptr)))::value_type;

template <auto TImpl, class TChar>
int utf8_main(int argc, const TChar* const* argv) {
  auto utf8 = public_api::make_utf8_argv(argc, argv);
  // TODO: error handling
  return TImpl(*std::move(utf8));
}

template <class T>
struct function_argument_type_t {
  constexpr explicit function_argument_type_t(int (*)(T)) {
  }
  using type = std::remove_cvref_t<T>;
};

template <class T>
struct argument_parsing_traits {
  using type = gnu_style_parsing_traits;
};
template <class T>
  requires requires { typename T::parsing_traits; }
struct argument_parsing_traits<T> {
  static_assert(
    parsing_traits<typename T::parsing_traits>,
    "cli_parsing_traits is defined, but the type does not satisfy the "
    "parsing_traits concept");
  using type = T::parsing_traits;
};
template <class T>
using argument_parsing_traits_t = argument_parsing_traits<T>::type;

template <auto TImpl, class TArgv>
int parsed_main(TArgv&& argv) {
  using Parsed = decltype(function_argument_type_t {TImpl})::type;
  static_assert(
    std::same_as<std::invoke_result_t<decltype(TImpl), Parsed>, int>);

  auto parsed = magic_args::parse<Parsed>(std::forward<TArgv>(argv));
  if (parsed) [[likely]] {
    return TImpl(*std::move(parsed));
  }

  return is_error(parsed.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
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