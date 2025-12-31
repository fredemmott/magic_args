// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_INVOCABLE_DECLARATIONS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_INVOCABLE_DECLARATIONS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "declarations.hpp"
#endif

namespace magic_args::inline public_api {

template <class T>
concept invocable_subcommand = subcommand<T>
  && requires(typename T::arguments_type&& args) { T::main(std::move(args)); };

template <invocable_subcommand T>
using subcommand_return_t = std::remove_cvref_t<
  std::invoke_result_t<decltype(T::main), typename T::arguments_type&&>>;

template <class T, class U>
concept compatible_invocable_subcommand
  = invocable_subcommand<T> && invocable_subcommand<U>
  && std::same_as<subcommand_return_t<T>, subcommand_return_t<U>>;

template <subcommand First, compatible_invocable_subcommand<First>... Rest>
struct invocable_subcommands_list {
  using return_type = std::remove_cvref_t<subcommand_return_t<First>>;
};

}// namespace magic_args::inline public_api

#endif
