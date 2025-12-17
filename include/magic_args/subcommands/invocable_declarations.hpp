// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include "declarations.hpp"
#endif

namespace magic_args::inline public_api {

template <class T>
concept invocable_subcommand = subcommand<T>
  && requires(typename T::arguments_type&& args) { T::main(std::move(args)); };

template <class T, class TOther>
concept compatible_invocable_subcommand
  = invocable_subcommand<T> && invocable_subcommand<TOther>
  && std::same_as<
      std::invoke_result_t<decltype(T::main), typename T::arguments_type&&>,
      std::invoke_result_t<
        decltype(TOther::main),
        typename TOther::arguments_type&&>>;
}// namespace magic_args::inline public_api
