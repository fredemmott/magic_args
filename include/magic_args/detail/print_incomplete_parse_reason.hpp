// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PRINT_INCOMPLETE_PARSE_REASON_HPP
#define MAGIC_ARGS_DETAIL_PRINT_INCOMPLETE_PARSE_REASON_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/incomplete_parse_reason.hpp>
#include "print.hpp"
#include "usage.hpp"
#endif

namespace magic_args::detail {
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const help_requested&,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE* errorStream) {
  show_usage<Traits, T>(outputStream, argv);
}

template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const version_requested&,
  [[maybe_unused]] argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  [[maybe_unused]] FILE* errorStream) {
  if constexpr (has_version<T>) {
    detail::println(outputStream, "{}", T::version);
  } else {
    throw std::logic_error(
      "magic_args: somehow got version_requested without a version");
  }
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const missing_required_argument& r,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: Missing required argument `{}`",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mName);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const missing_argument_value& r,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: option `{}` requires a value",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mName);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const invalid_argument& arg,
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
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const invalid_argument_value& r,
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

template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const incomplete_parse_reason_t& reason,
  argv_range auto&& argv,
  FILE* outputStream,
  FILE* errorStream) {
  std::visit(
    [=]<class R>(R&& it) {
      detail::print_incomplete_parse_reason<Traits, T>(
        std::forward<R>(it), argv, outputStream, errorStream);
      if constexpr (std::decay_t<R>::is_error) {
        detail::print(errorStream, "\n\n");
        show_usage<Traits, T>(errorStream, argv);
      }
    },
    reason);
}

}// namespace magic_args::detail

#endif
