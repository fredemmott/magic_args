// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SUBCOMMANDS_INSPECTION_HPP
#define MAGIC_ARGS_SUBCOMMANDS_INSPECTION_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/constexpr_strings.hpp>
#include "declarations.hpp"
#endif

#ifdef _WIN32
#define MAGIC_ARGS_DO_PRAGMA(x) _Pragma(#x)
#define MAGIC_ARGS_LINKER_EXPORT(SYMBOL_NAME) \
  MAGIC_ARGS_DO_PRAGMA(comment(linker, "/EXPORT:" #SYMBOL_NAME))
#define MAGIC_ARGS_EXPORT(SYMBOL_NAME) \
  MAGIC_ARGS_LINKER_EXPORT(SYMBOL_NAME) __declspec(dllexport)
#else
#define MAGIC_ARGS_EXPORT(SYMBOL_NAME) __attribute__((visibility("default")))
#endif

namespace magic_args::detail {

template <class... Args>
struct subcommands_list_t {
  struct first_must_be_subcommand_or_root_traits {};
  static constexpr first_must_be_subcommand_or_root_traits value {};
};

template <subcommand First, subcommand... Rest>
struct subcommands_list_t<First, Rest...> {
  static constexpr auto make() {
    constexpr auto first = [] {
      constexpr auto view = std::string_view {First::name};
      std::array<char, view.size() + 1> arr {};
      std::ranges::copy(view, arr.begin());
      return arr;
    }();

    if constexpr (sizeof...(Rest) == 0) {
      return first;
    } else {
      using namespace constexpr_strings;
      constexpr auto rest = subcommands_list_t<Rest...>::make();
      return concat<concat_byte_array_traits>(first, rest).get_buffer();
    }
  }

  static constexpr auto value = make();
};

template <root_command_traits Traits, subcommand First, subcommand... Rest>
struct subcommands_list_t<Traits, First, Rest...>
  : subcommands_list_t<First, Rest...> {};

template <class... Args>
struct introspectable_subcommands_list_t {
  static constexpr auto buffer = [] {
    using namespace constexpr_strings;
    return concat<concat_byte_array_traits>(
             subcommands_list_t<Args...>::value, std::array {'\0'})
      .get_buffer();
  }();

  static constexpr auto size = buffer.size();

  struct wrapper_t {
    char data[size];
  };

  static constexpr auto wrapper
    = []<std::size_t... Is>(std::index_sequence<Is...>) -> wrapper_t {
    return {{buffer[Is]...}};
  }(std::make_index_sequence<size> {});
};

}// namespace magic_args::detail

#define MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(...) \
  extern "C" MAGIC_ARGS_EXPORT(magic_args_subcommands_list) \
    const auto magic_args_subcommands_list \
    = magic_args::detail::introspectable_subcommands_list_t< \
      __VA_ARGS__>::wrapper;

#endif
