// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PRINT_INCOMPLETE_PARSE_REASON_HPP
#define MAGIC_ARGS_DETAIL_PRINT_INCOMPLETE_PARSE_REASON_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/console_output.hpp>
#include <magic_args/incomplete_parse_reason.hpp>
#include "usage.hpp"
#endif

namespace magic_args::detail {
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const help_requested&,
  argv_range auto&& argv,
  console_output auto& output) {
  show_usage<Traits, T>(output.out, argv);
}

template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const version_requested&,
  [[maybe_unused]] argv_range auto&& argv,
  [[maybe_unused]] console_output auto& output) {
  if constexpr (has_version<T>) {
    output.out.println("{}", T::version);
  } else {
    throw std::logic_error(
      "magic_args: somehow got version_requested without a version");
  }
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const missing_required_argument& r,
  argv_range auto&& argv,
  console_output auto& output) {
  output.error.print(
    "{}: Missing required argument `{}`",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mName);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const missing_argument_value& r,
  argv_range auto&& argv,
  console_output auto& output) {
  output.error.print(
    "{}: option `{}` requires a value",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mArgvMember);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const unrecognized_option& arg,
  argv_range auto&& argv,
  console_output auto& output) {
  output.error.print(
    "{}: Unrecognized option: {}",
    get_prefix_for_user_messages<Traits>(argv),
    arg.mSource.mArg);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const too_many_arguments& arg,
  argv_range auto&& argv,
  console_output auto& output) {
  output.error.print(
    "{}: Unexpected argument: {}",
    get_prefix_for_user_messages<Traits>(argv),
    arg.mSource.mArg);
}
template <parsing_traits Traits, class T>
void print_incomplete_parse_reason(
  const invalid_argument_value& r,
  argv_range auto&& argv,
  console_output auto& output) {
  output.error.print(
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
  console_output auto& output) {
  std::visit(
    [&]<class R>(R&& it) {
      detail::print_incomplete_parse_reason<Traits, T>(
        std::forward<R>(it), argv, output);
      if constexpr (std::decay_t<R>::is_error) {
        output.error.print("\n\n");
        show_usage<Traits, T>(output.error, argv);
      }
    },
    reason);
}

}// namespace magic_args::detail

#endif
