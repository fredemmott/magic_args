// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_STATIC_ASSERT_NOT_AN_ENUM_HPP
#define MAGIC_ARGS_STATIC_ASSERT_NOT_AN_ENUM_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "config.hpp"
#endif

#ifdef MAGIC_ARGS_DISABLE_ENUM
#include <type_traits>

namespace magic_args::detail {
template <class T>
void static_assert_not_an_enum() {
  constexpr auto ArgumentIsNotAnEnum = !std::is_enum_v<std::remove_cvref_t<T>>;
#if !__has_include(<magic_enum/magic_enum.hpp>)
  static_assert(
    ArgumentIsNotAnEnum,
    "enum support in magic_args requires <magic_enum/magic_enum.hpp> in the "
    "compiler include path, but that header is not available");
#else
  static_assert(
    ArgumentIsNotAnEnum,
    "enum support in magic_args has been disabled via MAGIC_ARGS_DISABLE_ENUM, "
    "even though <magic_enum/magic_enum.hpp> is available");
#endif
}
}// namespace magic_args::detail

#endif
#endif
