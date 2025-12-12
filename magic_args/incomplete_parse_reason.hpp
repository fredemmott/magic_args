// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#endif

#include <variant>

namespace magic_args::inline public_api {
inline namespace incomplete_parse_reasons {

struct help_requested {
  static constexpr bool is_error = false;
  static constexpr bool user_requested = true;
};
struct version_requested {
  static constexpr bool is_error = false;
  static constexpr bool user_requested = true;
};
struct missing_required_argument {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;
};
struct missing_argument_value {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;
};
struct invalid_argument {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;
};
struct invalid_argument_value {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;

  struct source_t {
    std::vector<std::string> mArgvSlice;
    std::string mName;
    std::string mValue;
    constexpr bool operator==(const source_t&) const noexcept = default;
  };

  // Automatically populated by the framework; if you fill this out, an
  // exception will be thrown
  source_t mSource;
};
struct invalid_encoding {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;
};

}// namespace incomplete_parse_reasons

template <class T>
concept incomplete_parse_reason = requires {
  T::is_error;
  T::user_requested;
  { T::is_error } -> detail::same_as_ignoring_cvref<bool>;
  { T::user_requested } -> detail::same_as_ignoring_cvref<bool>;
};
}// namespace magic_args::inline public_api

namespace magic_args::detail {
// TODO (C++26): use `template <class> concept` instead when we drop C++23
// support
template <class T>
using incomplete_parse_reason_concept_trait
  = std::bool_constant<incomplete_parse_reason<T>>;

template <
  template <class...> class TContainer,
  template <class> class TConstraint>
struct constrained_pack {
  template <class... Ts>
    requires(TConstraint<Ts>::value && ...)
  using type = TContainer<Ts...>;
};
}// namespace magic_args::detail

namespace magic_args::inline public_api {

using incomplete_parse_reason_t = detail::constrained_pack<
  std::variant,
  detail::incomplete_parse_reason_concept_trait>::
  type<
    help_requested,
    version_requested,
    missing_required_argument,
    missing_argument_value,
    invalid_argument,
    invalid_argument_value,
    invalid_encoding>;

}// namespace magic_args::inline public_api