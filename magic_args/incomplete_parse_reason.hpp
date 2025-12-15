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
  constexpr bool operator==(const help_requested&) const = default;
};
struct version_requested {
  static constexpr bool is_error = false;
  static constexpr bool user_requested = true;
  constexpr bool operator==(const version_requested&) const = default;
};
struct missing_required_argument {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;

  struct source_t {
    std::string mName;
    constexpr bool empty() const noexcept {
      return mName.empty();
    }
    constexpr bool operator==(const source_t&) const = default;
  };
  source_t mSource;
  constexpr bool operator==(const missing_required_argument&) const = default;
};
struct missing_argument_value {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;

  struct source_t {
    std::string mName;
    std::string mArgvMember;

    constexpr bool empty() const noexcept {
      return mName.empty() && mArgvMember.empty();
    }
    constexpr bool operator==(const source_t&) const = default;
  };

  source_t mSource;
  constexpr bool operator==(const missing_argument_value&) const = default;
};
struct invalid_argument {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;

  enum class kind {
    Option,
    Positional,
  };
  struct source_t {
    std::string mArg;
    constexpr bool operator==(const source_t&) const = default;
  };

  kind mKind {};
  source_t mSource;
  constexpr bool operator==(const invalid_argument&) const = default;
};

struct invalid_argument_value {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;

  struct source_t {
    std::vector<std::string> mArgvSlice;
    std::string mName;
    std::string mValue;

    constexpr bool empty() const noexcept {
      return mArgvSlice.empty() && mName.empty() && mValue.empty();
    }
    constexpr bool operator==(const source_t&) const = default;
  };

  // Automatically populated by the framework; if you fill this out, an
  // exception will be thrown
  source_t mSource;
  constexpr bool operator==(const invalid_argument_value&) const = default;
};
struct invalid_encoding {
  static constexpr bool is_error = true;
  static constexpr bool user_requested = false;
  constexpr bool operator==(const invalid_encoding&) const = default;
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

template <incomplete_parse_reason T>
[[nodiscard]]
constexpr bool is_error(T&&) noexcept {
  return std::decay_t<T>::is_error;
}

template <incomplete_parse_reason... Ts>
[[nodiscard]]
constexpr bool is_error(const std::variant<Ts...>& reason) {
  return std::visit(
    []<class T>(T&&) { return std::decay_t<T>::is_error; }, reason);
}

}// namespace magic_args::inline public_api