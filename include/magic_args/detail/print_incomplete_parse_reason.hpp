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
template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const help_requested&,
  const program_info& help,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE* errorStream) {
  show_usage<T, Traits>(outputStream, argv, help);
}

template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const version_requested&,
  const program_info& help,
  [[maybe_unused]] argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE* errorStream) {
  detail::println(outputStream, "{}", help.mVersion);
}
template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const missing_required_argument& r,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: Missing required argument `{}`",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mName);
}
template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const missing_argument_value& r,
  [[maybe_unused]] const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: option `{}` requires a value",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mName);
}
template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const invalid_argument& arg,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  switch (arg.mKind) {
    case invalid_argument::kind::Option:
      detail::print(
        errorStream,
        "{}: Unrecognized option: {}",
        get_prefix_for_user_messages<Traits>(argv),
        arg.mSource.mArg);
      break;
    case invalid_argument::kind::Positional:
      detail::print(
        errorStream,
        "{}: Unexpected argument: {}",
        get_prefix_for_user_messages<Traits>(argv),
        arg.mSource.mArg);
      break;
  }
}
template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const invalid_argument_value& r,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: `{}` is not a valid value for `{}` (seen: `{}`)",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mValue,
    r.mSource.mName,
    // 2025-12-13: no join_with on Apple Clang
    std::ranges::fold_left(
      std::views::drop(r.mSource.mArgvSlice, 1),
      r.mSource.mArgvSlice.front(),
      [](auto acc, auto it) { return std::format("{} {}", acc, it); }));
}

template <class T, parsing_traits Traits>
void print_incomplete_parse_reason(
  const incomplete_parse_reason_t& reason,
  const program_info& help,
  argv_range auto&& argv,
  FILE* outputStream,
  FILE* errorStream) {
  std::visit(
    [=, &help]<class R>(R&& it) {
      detail::print_incomplete_parse_reason<T, Traits>(
        std::forward<R>(it), help, argv, outputStream, errorStream);
      if constexpr (std::decay_t<R>::is_error) {
        detail::print(errorStream, "\n\n");
        show_usage<T, Traits>(errorStream, argv, help);
      }
    },
    reason);
}

}// namespace magic_args::detail
