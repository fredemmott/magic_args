// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_GET_ARGUMENT_DEFINITION_HPP
#define MAGIC_ARGS_DETAIL_GET_ARGUMENT_DEFINITION_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include "reflection.hpp"
#endif

#include <concepts>
#include <string>

namespace magic_args::detail {

template <class T>
concept static_basic_argument = requires {
  typename T::value_type;
  T::default_value();
  { T::behavior } -> same_as_ignoring_cvref<Behavior>;
  { T::name } -> explicitly_convertible_to<std::string_view>;
};

template <class T>
concept static_basic_option = static_basic_argument<T> && requires {
  { T::short_name } -> same_as_ignoring_cvref<std::string_view>;
};

template <class T>
concept static_basic_positional_argument
  = static_basic_argument<T> && !static_basic_option<T>;

template <class TArgs, std::size_t I, parsing_traits Traits>
static constexpr auto get_argument_name_by_index() {
  using member_type = member_type_by_index<TArgs, I>;
  constexpr auto Input = [] {
    if constexpr (!basic_argument<member_type>) {
      return Traits::template normalize_option_name<
        member_name_by_index<TArgs, I>>();
    } else {
      constexpr auto value = std::get<I>(tie_struct(TArgs {})).mName;
      if constexpr (!value.empty()) {
        return value;
      } else if constexpr (is_option(member_type::behavior)) {
        return Traits::template normalize_option_name<
          member_name_by_index<TArgs, I>>();
      } else {
        return Traits::template normalize_positional_argument_name<
          member_name_by_index<TArgs, I>>();
      }
    }
  }();
  constexpr auto Array = []<std::size_t N>(const auto& v) {
    std::array<char, N> ret {};
    std::ranges::copy(std::string_view {v}, ret.begin());
    return ret;
  }.template operator()<std::string_view {Input}.size()>(Input);
  return constexpr_strings::identity_t<Array> {};
}

// Fallback: option or flag with everything inferred
template <class T, std::size_t I, parsing_traits Traits>
struct argument_definition_t {
  using member_type = member_type_by_index<T, I>;
  using value_type = member_type;
  static_assert(!basic_argument<value_type>);

  static constexpr value_type default_value() {
    return default_value_by_index<T, I>();
  }

  static constexpr auto behavior
    = std::same_as<value_type, bool> ? Behavior::Flag : Behavior::Option;

  static constexpr auto name = get_argument_name_by_index<T, I, Traits>();
  static constexpr std::string_view short_name {};
};

// Positional arguments
template <class T, std::size_t I, parsing_traits Traits>
  requires basic_argument<member_type_by_index<T, I>>
  && (!basic_option<member_type_by_index<T, I>>)
struct argument_definition_t<T, I, Traits> {
  using member_type = member_type_by_index<T, I>;
  using value_type = member_type::value_type;

  static constexpr value_type default_value() {
    return default_value_by_index<T, I>().mValue;
  }

  static constexpr auto behavior = member_type::behavior;
  static constexpr auto name = get_argument_name_by_index<T, I, Traits>();
};

// Options
template <class T, std::size_t I, parsing_traits Traits>
  requires basic_option<member_type_by_index<T, I>>
struct argument_definition_t<T, I, Traits> {
  using member_type = member_type_by_index<T, I>;
  using value_type = member_type::value_type;

  static constexpr value_type default_value() {
    return default_value_by_index<T, I>().mValue;
  }

  static constexpr auto behavior = member_type::behavior;
  static constexpr auto name = get_argument_name_by_index<T, I, Traits>();

  static constexpr auto short_name_buffer = [] {
    constexpr auto Raw = default_value_by_index<T, I>().mShortName;
    if constexpr (Raw.empty()) {
      return constexpr_strings::empty_v;
    } else {
      constexpr auto Arr = []<std::size_t N>(const auto& v) {
        std::array<char, N> ret {};
        std::ranges::copy(std::string_view {v}, ret.begin());
        return ret;
      }.template operator()<std::string_view {Raw}.size()>(Raw);
      return constexpr_strings::identity_t<Arr> {};
    }
  }();
  static constexpr std::string_view short_name {short_name_buffer};
};
}// namespace magic_args::detail

#endif