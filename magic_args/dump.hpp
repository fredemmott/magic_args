// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "detail/print.hpp"
#include "detail/reflection.hpp"
#endif

namespace magic_args::detail {

template <std::formattable<char> T>
auto formattable_argument_value(const T& arg) {
  return arg;
}

template <class T>
std::string formattable_argument_value(const std::optional<T>& arg)
  requires requires { formattable_argument_value(arg.value()); }
{
  if (arg.has_value()) {
    return std::format("{}", formattable_argument_value(arg.value()));
  }
  return "[nullopt]";
}

template <basic_argument T>
auto formattable_argument_value(const T& arg)
  requires requires { formattable_argument_value(arg.mValue); }
{
  return formattable_argument_value(arg.mValue);
}

}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <class T>
void dump(const T& args, FILE* output = stdout) {
  using namespace detail;
  const auto tuple = tie_struct(args);

  []<std::size_t... I>(
    const auto& args, FILE* output, std::index_sequence<I...>) {
    (detail::println(
       output,
       "{:29} `{}`",
       member_name<T, I>,
       formattable_argument_value(get<I>(args))),
     ...);
  }(tuple,
    output,
    std::make_index_sequence<std::tuple_size_v<decltype(tuple)>> {});
}

}// namespace magic_args::inline public_api
