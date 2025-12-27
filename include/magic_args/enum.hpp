// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_ENUM_HPP
#define MAGIC_ARGS_ENUM_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/config.hpp"
#include "detail/from_string.hpp"
#include "detail/to_formattable.hpp"
#include "detail/usage.hpp"
#endif

#ifndef MAGIC_ARGS_DISABLE_ENUM
#include <type_traits>

// For now, this is built with magic_enum; when we start targetting
// C++26, we should investigate (also?) supporting reflection.
#include <magic_enum/magic_enum.hpp>

namespace magic_args::detail {
template <class T>
concept cpp_enum = std::is_enum_v<std::decay_t<T>>;

template <cpp_enum T>
  requires(!has_adl_formattable_argument_value<T>)
struct to_formattable_t<T> {
  static constexpr auto operator()(T&& v) {
    return magic_enum::enum_name(std::forward<T>(v));
  }
};

template <cpp_enum T>
  requires(!has_adl_from_string_argument<T>)
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

template <basic_argument TArg>
  requires std::is_enum_v<typename std::decay_t<TArg>::value_type>
struct get_argument_help_t<TArg> {
  static std::string operator()(const TArg& argDef) {
    if (!argDef.mHelp.empty()) {
      return std::string {argDef.mHelp};
    }

    const auto values
      = magic_enum::enum_values<typename std::decay_t<TArg>::value_type>();
    if (values.empty()) {
      return {};
    }

    if (values.size() == 2) {
      return std::format(
        "`{}` or `{}`", to_formattable(values[0]), to_formattable(values[1]));
    }

    const auto last = values[values.size() - 1];
    std::string ret;
    for (auto&& value: values) {
      const auto name = to_formattable(value);
      if (ret.empty()) {
        ret = std::format("`{}`", name);
        continue;
      }
      if (value == last) {
        ret.append(std::format(", or `{}`", name));
      } else {
        ret.append(std::format(", `{}`", name));
      }
    }
    return ret;
  }
};

}// namespace magic_args::detail

#endif
#endif