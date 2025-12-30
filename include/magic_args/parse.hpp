// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_PARSE_HPP
#define MAGIC_ARGS_PARSE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "console_output.hpp"
#include "detail/print_incomplete_parse_reason.hpp"
#include "parse_silent.hpp"
#endif

#include <expected>

namespace magic_args::inline public_api {

template <class T, console_output TConsole = print_console_output>
std::expected<T, incomplete_parse_reason_t> parse(
  detail::argv_range auto&& argv,
  TConsole&& console = {}) {
  const auto ret = parse_silent<T>(std::forward<decltype(argv)>(argv));
  if (!ret) [[unlikely]] {
    using Traits = detail::parsing_traits_for_args_t<T>;
    detail::print_incomplete_parse_reason<Traits, T>(
      ret.error(), argv, console);
  }
  return ret;
}

}// namespace magic_args::inline public_api

#endif