// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PRINT_UTF8_ERROR_HPP
#define MAGIC_ARGS_DETAIL_PRINT_UTF8_ERROR_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/console_output.hpp>
#include "encoding.hpp"
#endif

namespace magic_args::detail {
void print_utf8_error(const invalid_parameter_t&, text_sink auto&& console) {
  console.println(
    "Unable to convert argv to UTF-8 because an invalid argc/argv were "
    "passed to the program by the operating system");
}

void print_utf8_error(
  const only_utf8_supported_t& e,
  text_sink auto&& console) {
  console.println(
    "This program requires input in UTF-8, however the input is in "
    "`{}`",
    e.detected_encoding);
}

void print_utf8_error(
  const encoding_not_supported_t& e,
  text_sink auto&& console) {
  console.println(
    "argv is in `{0}`, but this program does not support converting "
    "from `{0}` to UTF-8",
    e.detected_encoding);
}

void print_utf8_error(
  const encoding_conversion_failed_t& e,
  text_sink auto&& console) {
  console.println(
    "Converting from `{}` to UTF-8 failed ({})",
    e.detected_encoding,
    e.error_code.message());
}

void print_utf8_error(
  const range_construction_failed_t& e,
  text_sink auto&& console) {
  console.println(
    "Unable to create argv from command line: {}", e.error_code.message());
}

void print_utf8_error(
  const make_utf8_argv_error_t& error,
  text_sink auto&& console) {
  std::visit([&](auto& it) { print_utf8_error(it, console); }, error);
}
}// namespace magic_args::detail

#endif
