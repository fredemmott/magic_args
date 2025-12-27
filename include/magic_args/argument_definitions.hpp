// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_ARGUMENT_DEFINITIONS_HPP
#define MAGIC_ARGS_ARGUMENT_DEFINITIONS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/concepts.hpp"
#include "detail/definition_tags.hpp"
#endif

#include <type_traits>

namespace magic_args::detail {
struct empty_t final {};
}// namespace magic_args::detail

namespace magic_args::inline public_api {

/** An argument with additional metadata.
 *
 * This is the implementation of `magic_args::option<T>`,
 * `flag<T>`, `mandatory_positional_argument<T>` and similar types.
 *
 * I've used this template-with-tag approach instead of inheritance so that:
 *
 * - initializer lists work, e.g
 *   `option<std::string> foo { defaultValue, "name", "helpText" };`
 * - designated initializers work, e.g.
 *   `option<std::string> foo { .mShortName = "f" };`
 *
 * The main alternative was using macros :'(
 */
template <class T, detail::definition_tags::any TTag>
struct decorated_argument final {
  using value_type = T;

  static constexpr bool is_std_optional = detail::std_optional<T>;
  static constexpr bool is_required = std::
    same_as<TTag, detail::definition_tags::mandatory_positional_argument_t>;
  // This includes both `--option==value` (the `option` type), and `--flag`,
  // `--counted-flag --counted-flag`, etc
  static constexpr bool is_option = detail::definition_tags::any_option<TTag>;
  static constexpr bool is_positional_argument
    = detail::definition_tags::any_positional_argument<TTag>;
  static_assert(is_option == !is_positional_argument);

  T mValue {};
  std::string_view mName;
  std::string_view mHelp;

  // MS attribute takes priority because under MSVC, [[no_unique_address]
  // is recognized but a no-op, to avoid breaking ABI with libraries compiled
  // before it was supported (where the standard 'unrecognized attribute does
  // nothing' behavior kept the code valid).
#if _MSC_VER
  [[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
  [[no_unique_address]]
#endif
  std::conditional_t<is_option, std::string_view, detail::empty_t> mShortName;

  template <class Self>
  [[nodiscard]] constexpr decltype(auto) value(this Self&& self) noexcept(
    !is_std_optional) {
    if constexpr (is_std_optional) {
      return std::forward<Self>(self).mValue.value();
    } else {
      return std::forward<Self>(self).mValue;
    }
  }

  operator T() const noexcept {
    return mValue;
  }

  template <class U>
    requires std::assignable_from<T&, U>
  auto& operator=(U&& rhs) {
    mValue = std::forward<U>(rhs);
    return *this;
  }

  constexpr bool operator==(const decorated_argument&) const noexcept = default;
  template <class U>
    requires(!detail::same_as_ignoring_cvref<decorated_argument, U>)
    && std::equality_comparable_with<T, U>
  constexpr bool operator==(U&& rhs) const noexcept {
    return mValue == std::forward<U>(rhs);
  }

  template <class Self>
  [[nodiscard]]
  decltype(auto) operator*(this Self&& self) noexcept(!is_std_optional) {
    return std::forward<Self>(self).value();
  }

  template <class Self>
  decltype(auto) operator->(this Self&& self) {
    if constexpr (is_std_optional) {
      return std::forward<Self>(self).mValue;
    } else {
      return &std::forward<Self>(self).mValue;
    }
  }

  [[nodiscard]] constexpr bool has_value() const noexcept
    requires is_std_optional
  {
    return mValue.has_value();
  };

  /** "Was the argument provided?" for common types.
   *
   * Implicit bool conversions can be surprising, so, this is restricted
   * to types where:
   *
   * - it's expected
   * - it's well-defined
   * - semantics are consistent
   *
   * For example, if this just allowed `std::convertible_to<bool>`, a
   * `magic_args::optional<std::string>` would be bool-convertible, but it
   * would mean 'provided or empty', not 'was this argument provided?'.
   *
   * If you want to support this for another type, you probably want to use
   * `std::optional<T>` instead of `T`.
   */
  constexpr operator bool() const noexcept
    requires is_std_optional
    || std::same_as<TTag, detail::definition_tags::flag_t>
    || std::same_as<TTag, detail::definition_tags::counted_flag_t>
  {
    return static_cast<bool>(mValue);
  }
};

// e.g. `--foo=bar`
template <class T>
using option = decorated_argument<T, detail::definition_tags::option_t>;
// e.g. `--quiet` or `--help`
using flag = decorated_argument<bool, detail::definition_tags::flag_t>;
// e.g. for `-vvv` -> triple-verbose
using counted_flag
  = decorated_argument<std::size_t, detail::definition_tags::counted_flag_t>;

// e.g. `MyApp [FILE]`
template <class T>
using optional_positional_argument = decorated_argument<
  T,
  detail::definition_tags::optional_positional_argument_t>;

// e.g. `MyApp FILE`
template <class T>
using mandatory_positional_argument = decorated_argument<
  T,
  detail::definition_tags::mandatory_positional_argument_t>;

}// namespace magic_args::inline public_api

#endif