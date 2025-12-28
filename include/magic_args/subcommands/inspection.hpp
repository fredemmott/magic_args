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

template <root_command_traits Traits, subcommand First, subcommand... Rest>
struct subcommands_list_t<Traits, First, Rest...> {
  static constexpr auto value = [] {
    using namespace constexpr_strings;
    constexpr auto first
      = append_null_t<subcommand_name_t<Traits, First>::value>::value;
    if constexpr (sizeof...(Rest) == 0) {
      return first;
    } else {
      constexpr auto rest = subcommands_list_t<Traits, Rest...>::value;
      return concat_t<first, rest>::value;
    }
  }();
};

template <subcommand First, subcommand... Rest>
struct subcommands_list_t<First, Rest...>
  : subcommands_list_t<gnu_style_parsing_traits, First, Rest...> {};

template <class... Args>
struct introspectable_subcommands_list_t {
  static constexpr auto value = [] {
    return constexpr_strings::append_null_t<
      subcommands_list_t<Args...>::value>::value;
  }();

  static constexpr auto size = value.size();

  // This is what we actually put in the symbol table
  struct wrapper_t {
    char data[size] {};
  };

  static constexpr auto wrapper
    = []<std::size_t... Is>(std::index_sequence<Is...>) -> wrapper_t {
    return {{value[Is]...}};
  }(std::make_index_sequence<size> {});
};

}// namespace magic_args::detail

#define MAGIC_ARGS_MAKE_SUBCOMMANDS_INSPECTABLE(...) \
  extern "C" MAGIC_ARGS_EXPORT(magic_args_subcommands_list) \
    const auto magic_args_subcommands_list \
    = magic_args::detail::introspectable_subcommands_list_t< \
      __VA_ARGS__>::wrapper;

#endif
