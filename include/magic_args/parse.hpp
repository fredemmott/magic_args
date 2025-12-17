// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_PARSE_HPP
#define MAGIC_ARGS_PARSE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/print_incomplete_parse_reason.hpp"
#include "detail/usage.hpp"
#include "gnu_style_parsing_traits.hpp"
#include "parse_silent.hpp"
#include "program_info.hpp"
#endif

#include <expected>
#include <filesystem>
#include <format>
#include <ranges>

namespace magic_args::inline public_api {

template <parsing_traits Traits, class T>
std::expected<T, incomplete_parse_reason_t> parse(
  detail::argv_range auto&& argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret
    = parse_silent<Traits, T>(std::forward<decltype(argv)>(argv), help);
  if (!ret) [[unlikely]] {
    detail::print_incomplete_parse_reason<T, Traits>(
      ret.error(), help, argv, outputStream, errorStream);
  }
  return ret;
}

template <class T>
std::expected<T, incomplete_parse_reason_t> parse(
  detail::argv_range auto&& argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse<gnu_style_parsing_traits, T>(
    std::forward<decltype(argv)>(argv), help, outputStream, errorStream);
}

}// namespace magic_args::inline public_api

#endif