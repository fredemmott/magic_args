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
  { T::name } -> same_as_ignoring_cvref<std::string_view>;
};

template <class T>
concept static_basic_option
  = static_basic_argument<T> && is_option(T::behavior) && requires {
      { T::short_name } -> same_as_ignoring_cvref<std::string_view>;
    };
template <class T>
concept static_flag
  = static_basic_option<T> && (T::behavior == Behavior::Flag) && requires {
      { T::negated_flag_name } -> same_as_ignoring_cvref<std::string_view>;
    };

template <class T>
concept static_basic_positional_argument
  = static_basic_argument<T> && is_positional_argument(T::behavior);

template <class TArgs, std::size_t I, parsing_traits Traits>
static constexpr auto make_argument_name_by_index() {
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
  return []<std::size_t N>(const auto& v) {
    std::array<char, N> ret {};
    std::ranges::copy(std::string_view {v}, ret.begin());
    return ret;
  }.template operator()<std::string_view {Input}.size()>(Input);
}
template <class TArgs, std::size_t I, parsing_traits Traits>
static constexpr const auto& get_argument_name_by_index() {
  static constexpr auto ret = make_argument_name_by_index<TArgs, I, Traits>();
  return ret;
}

template <class TArgs, std::size_t I, parsing_traits Traits, Behavior TBehavior>
static constexpr std::string_view get_negated_flag_name_by_index() {
  if constexpr (
    parsing_traits_with_negated_flags<Traits> && TBehavior == Behavior::Flag) {
    static constexpr auto buffer = Traits::template negated_flag_name<
      get_argument_name_by_index<TArgs, I, Traits>()>();
    return std::string_view {buffer};
  } else {
    return {};
  }
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

  static constexpr std::string_view name {
    get_argument_name_by_index<T, I, Traits>()};
  static constexpr std::string_view short_name {};

  static constexpr auto negated_flag_name
    = get_negated_flag_name_by_index<T, I, Traits, behavior>();
};

// Explicit options, using a magic_args::option or related
template <class T, std::size_t I, parsing_traits Traits>
  requires basic_option<member_type_by_index<T, I>>
struct argument_definition_t<T, I, Traits> {
  using member_type = member_type_by_index<T, I>;
  using value_type = member_type::value_type;

  static constexpr value_type default_value() {
    return default_value_by_index<T, I>().mValue;
  }

  static constexpr auto behavior = member_type::behavior;
  static constexpr std::string_view name {
    get_argument_name_by_index<T, I, Traits>()};

  static constexpr auto short_name = [] -> std::string_view {
    constexpr auto Raw = default_value_by_index<T, I>().mShortName;
    if constexpr (Raw.empty()) {
      return {};
    } else {
      static constexpr auto Arr = []<std::size_t N>(const auto& v) {
        std::array<char, N> ret {};
        std::ranges::copy(std::string_view {v}, ret.begin());
        return ret;
      }.template operator()<std::string_view {Raw}.size()>(Raw);
      return std::string_view {Arr};
    }
  }();

  static constexpr auto negated_flag_name {
    get_negated_flag_name_by_index<T, I, Traits, behavior>()};
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
  static constexpr std::string_view name {
    get_argument_name_by_index<T, I, Traits>()};
};

}// namespace magic_args::detail

#endif