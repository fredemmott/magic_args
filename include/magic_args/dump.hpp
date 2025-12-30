// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DUMP_HPP
#define MAGIC_ARGS_DUMP_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "console_output.hpp"
#include "detail/reflection.hpp"
#include "detail/static_assert_not_an_enum.hpp"
#include "detail/to_formattable.hpp"
#endif

namespace magic_args::inline public_api {

template <class T, text_sink TOutput = print_text_sink>
void dump(const T& args, TOutput&& output = print_text_sink {stdout}) {
  using namespace detail;
  const auto tuple = tie_struct(args);

  [&]<std::size_t... I>(std::index_sequence<I...>) {
#ifdef MAGIC_ARGS_DISABLE_ENUM
    (static_assert_not_an_enum<std::tuple_element_t<I, decltype(tuple)>>(),
     ...);
#endif
    (output.println(
       "{:29} `{}`",
       std::string_view(member_name_by_index<T, I>),
       to_formattable(get<I>(tuple))),
     ...);
  }(std::make_index_sequence<std::tuple_size_v<decltype(tuple)>> {});
}

}// namespace magic_args::inline public_api

#endif
