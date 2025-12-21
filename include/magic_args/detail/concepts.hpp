// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_CONCEPTS_HPP
#define MAGIC_ARGS_DETAIL_CONCEPTS_HPP

#include <array>
#include <concepts>
#include <optional>
#include <string>

namespace magic_args::inline public_api {

template <class T>
concept basic_argument = requires(T v) {
  typename std::decay_t<T>::value_type;
  { v.mName } -> std::convertible_to<std::string_view>;
  { v.mHelp } -> std::convertible_to<std::string_view>;
};

template <class T>
concept basic_option = requires(T v) {
  typename std::decay_t<T>::value_type;
  { v.mName } -> std::convertible_to<std::string_view>;
  { v.mHelp } -> std::convertible_to<std::string_view>;
  { v.mShortName } -> std::convertible_to<std::string_view>;
};
}// namespace magic_args::inline public_api

namespace magic_args::detail {
template <class T>
concept has_description = requires {
  { T::description } -> std::convertible_to<std::string_view>;
};
template <class T>
concept has_version = requires {
  { T::version } -> std::convertible_to<std::string_view>;
};
template <class T>
concept has_examples = requires {
  requires std::ranges::input_range<decltype(T::examples)>;
  requires std::convertible_to<
    std::ranges::range_reference_t<decltype(T::examples)>,
    std::string_view>;
};

template <class T>
concept vector_like = requires { typename T::value_type; }
  && requires(T c, typename T::value_type v) { c.push_back(v); }
  && (!std::same_as<T, std::string>);

template <class T>
concept std_optional = requires { typename T::value_type; }
  && std::same_as<T, std::optional<typename T::value_type>>;

template <class T, class U>
concept same_as_ignoring_cvref
  = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

// A range of UTF-8 strings, in a form that can be iterated, randomly
// accessed, and the elements can be converted to a `std::string_view`
//
// e.g. `std::span<std::string_view>`, `std::vector<std::string>`,
// `std::views::counted(argv, argc)`
template <class T>
concept argv_range = std::ranges::random_access_range<T>
  && std::convertible_to<std::ranges::range_value_t<T>, std::string_view>;

// A randomly-accessible range R where all elements are of type T
//
// For example, an `std::span<std::string_view>` is a
// `random_access_range_of<std::string_view>`
template <class R, class T>
concept random_access_range_of = std::ranges::random_access_range<R>
  && std::same_as<std::ranges::range_value_t<R>, T>;

template <class T>
struct is_string_literal_t : std::false_type {};

template <std::size_t N>
struct is_string_literal_t<char const (&)[N]> : std::true_type {};

template <class T>
concept string_literal = is_string_literal_t<T>::value;

template <class T, class U>
concept explicitly_convertible_to = std::constructible_from<U, T>;
}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <class T>
concept parsing_traits = requires(std::string arg) {
  { T::long_arg_prefix } -> detail::string_literal;
  { T::short_arg_prefix } -> detail::string_literal;
  { T::value_separator } -> detail::string_literal;
  { T::long_help_arg } -> detail::string_literal;
  { T::short_help_arg } -> detail::string_literal;
  { T::version_arg } -> detail::string_literal;
  { T::single_char_short_args } -> detail::same_as_ignoring_cvref<bool>;

  {
    T::template normalize_option_name<std::array {'f', 'o', 'o'}>()
  } -> detail::explicitly_convertible_to<std::string_view>;
  {
    T::template normalize_positional_argument_name<std::array {'f', 'o', 'o'}>()
  } -> detail::explicitly_convertible_to<std::string_view>;

  requires requires {
    // Check that the functions are constexpr
    typename std::bool_constant<(
      (void)T::template normalize_option_name<std::array {'f', 'o', 'o'}>(),
      true)>;
    typename std::bool_constant<(
      (void)T::template normalize_positional_argument_name<std::array {
        'f', 'o', 'o'}>(),
      true)>;
  };
};

template <class T>
concept has_parsing_traits
  = requires { requires parsing_traits<typename T::parsing_traits>; };

}// namespace magic_args::inline public_api

#endif
