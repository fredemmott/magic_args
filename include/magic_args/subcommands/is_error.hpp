// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_IS_ERROR_HPP
#define MAGIC_ARGS_SUBCOMMANDS_IS_ERROR_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/overloaded.hpp>
#include <magic_args/incomplete_parse_reason.hpp>
#include "declarations.hpp"
#endif

namespace magic_args::inline public_api {

template <subcommand First, subcommand... Rest>
bool is_error(const incomplete_command_parse_reason_t<First, Rest...>& reason) {
  return std::visit(
    detail::overloaded {
      []<incomplete_parse_reason T>(const T&) { return T::is_error; },
      []<class T>(const incomplete_subcommand_parse_reason_t<T>& r) {
        return is_error(*r);
      },
    },
    reason);
}
}// namespace magic_args::inline public_api

#endif