// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/print.hpp"
#include "detail/reflection.hpp"
#include "detail/to_formattable.hpp"
#endif

#include <ranges>
#include <vector>

namespace magic_args::inline public_api {

template <class T>
void dump(const T& args, FILE* output = stdout) {
  using namespace detail;
  const auto tuple = tie_struct(args);

  []<std::size_t... I>(
    const auto& args, FILE* output, std::index_sequence<I...>) {
    (detail::println(
       output, "{:29} `{}`", member_name<T, I>, to_formattable(get<I>(args))),
     ...);
  }(tuple,
    output,
    std::make_index_sequence<std::tuple_size_v<decltype(tuple)>> {});
}

}// namespace magic_args::inline public_api
