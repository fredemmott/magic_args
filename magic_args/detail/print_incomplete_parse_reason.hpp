// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/incomplete_parse_reason.hpp>
#include <magic_args/program_info.hpp>
#include "print.hpp"
#include "usage.hpp"
#endif

namespace magic_args::detail {
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info& help,
  const std::string_view arg0,
  FILE* outputStream,
  [[maybe_unused]] FILE* errorStream,
  const help_requested&) {
  show_usage<T, Traits>(outputStream, arg0, help);
}

template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info& help,
  [[maybe_unused]] const std::string_view arg0,
  FILE* outputStream,
  [[maybe_unused]] FILE* errorStream,
  const version_requested&) {
  detail::println(outputStream, "{}", help.mVersion);
}
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info&,
  const std::string_view arg0,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream,
  const missing_required_argument& r) {
  detail::print(
    errorStream, "{}: Missing required argument `{}`", arg0, r.mSource.mName);
}
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info&,
  [[maybe_unused]] const std::string_view arg0,
  [[maybe_unused]] FILE* outputStream,
  [[maybe_unused]] FILE* errorStream,
  const missing_argument_value&) {
  // TODO
}
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info&,
  const std::string_view arg0,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream,
  const invalid_argument& arg) {
  switch (arg.mKind) {
    case invalid_argument::kind::Option:
      detail::print(
        errorStream, "{}: Unrecognized option: {}", arg0, arg.mSource.mArg);
      break;
    case invalid_argument::kind::Positional:
      detail::print(
        errorStream, "{}: Unexpected argument: {}", arg0, arg.mSource.mArg);
      break;
  }
}
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info&,
  [[maybe_unused]] const std::string_view arg0,
  [[maybe_unused]] FILE* outputStream,
  [[maybe_unused]] FILE* errorStream,
  const invalid_argument_value&) {
  // TODO
}
template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info&,
  [[maybe_unused]] const std::string_view arg0,
  [[maybe_unused]] FILE* outputStream,
  [[maybe_unused]] FILE* errorStream,
  const invalid_encoding&) {
  // TODO
}

template <class T, class Traits>
void print_incomplete_parse_reason(
  const program_info& help,
  const std::string_view arg0,
  FILE* outputStream,
  FILE* errorStream,
  const incomplete_parse_reason_t& reason) {
  std::visit(
    [=, &help]<class R>(R&& it) {
      detail::print_incomplete_parse_reason<T, Traits>(
        help,
        arg0.empty() ? arg0 : std::filesystem::path(arg0).stem().string(),
        outputStream,
        errorStream,
        std::forward<R>(it));
      if constexpr (std::decay_t<R>::is_error) {
        detail::print(errorStream, "\n\n");
        show_usage<T, Traits>(errorStream, arg0, help);
      }
    },
    reason);
}

}// namespace magic_args::detail
