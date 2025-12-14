// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#if (!defined(MAGIC_ARGS_ENUM_HPP)) && (!defined(MAGIC_ARGS_DISABLE_ENUM))
#define MAGIC_ARGS_ENUM_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/from_string.hpp"
#include "detail/to_formattable.hpp"
#endif

#include <type_traits>

// For now, this is built with magic_enum; when we start targetting
// C++26, we should investigate (also?) supporting reflection.
#include <magic_enum/magic_enum.hpp>

namespace magic_args::detail {
template <class T>
concept cpp_enum = std::is_enum_v<std::decay_t<T>>;

template <cpp_enum T>
struct to_formattable_t<T> {
  static constexpr auto operator()(T&& v) {
    return magic_enum::enum_name(std::forward<T>(v));
  }
};

template <cpp_enum T>
struct from_string_t<T> {
  static constexpr std::expected<void, invalid_argument_value> operator()(
    T& out,
    std::string_view arg) {
    const auto parsed = magic_enum::enum_cast<T>(arg);
    if (parsed) {
      out = *parsed;
      return {};
    }
    return std::unexpected {invalid_argument_value {}};
  }
};

/** Always show the defaults of enum fields.
 *
 * Without this specialization, the default would only be shown if
 * different to the default-constructed enum value.
 */
template <basic_option TArg>
  requires std::is_enum_v<typename TArg::value_type>
struct describe_default_value_t<TArg> {
  using TValue = typename TArg::value_type;
  static std::string operator()(const TValue value) {
    return std::format("{}", to_formattable(value));
  }
};

}// namespace magic_args::detail

#endif