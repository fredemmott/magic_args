// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/overloaded.hpp>
#include <magic_args/incomplete_parse_reason.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::inline public_api {

template <subcommand First, subcommand... Rest>
bool is_error(
  const std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>& reason) {
  return std::visit(
    detail::overloaded {
      [](const incomplete_command_parse_reason_t& r) { return is_error(r); },
      []<class T>(const incomplete_subcommand_parse_reason_t<T>& r) {
        return is_error(*r);
      },
    },
    reason);
}
}// namespace magic_args::inline public_api