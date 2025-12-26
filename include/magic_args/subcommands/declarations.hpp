// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_DECLARATIONS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_DECLARATIONS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/detail/constexpr_strings.hpp>
#include <magic_args/detail/reflection.hpp>
#include <magic_args/incomplete_parse_reason.hpp>
#include <magic_args/value_wrapper_t.hpp>
#endif

#include <concepts>
#include <string_view>
#include <type_traits>

namespace magic_args::inline public_api {

template <class T>
concept subcommand = requires {
  typename T::arguments_type;
  requires std::default_initializable<typename T::arguments_type>;
};

template <class T>
concept root_command_traits = (!subcommand<T>);

template <class Traits, class Command>
concept subcommand_naming_traits = root_command_traits<Traits> && requires {
  {
    Traits::
      template normalize_subcommand_name<Command, std::array {'f', 'o', 'o'}>()
  } -> detail::explicitly_convertible_to<std::string_view>;

  // Check that it's constexpr
  requires requires {
    typename std::bool_constant<(
      (void)Traits::template normalize_subcommand_name<
        Command,
        std::array {'f', 'o', 'o'}>(),
      true)>;
  };
};

template <class T>
concept explicitly_named = requires {
  T::name;
  std::string_view {T::name};

  // Check it's constexpr
  requires requires {
    typename std::bool_constant<(std::string_view {T::name}, true)>;
  };
};

}// namespace magic_args::inline public_api

namespace magic_args::detail {

template <auto Name>
struct remove_namespace_t {
  static constexpr auto value = [] {
    constexpr std::string_view view {Name};
    constexpr auto idx = view.rfind("::");
    if constexpr (idx == std::string_view::npos) {
      return Name;
    } else {
      constexpr auto begin = idx + 2;
      std::array<char, view.size() - begin> ret {};
      std::ranges::copy(view.substr(begin), ret.begin());
      return ret;
    }
  }();
};

template <class Traits, class T>
constexpr auto subcommand_name() {
  if constexpr (explicitly_named<T>) {
    constexpr std::string_view name {T::name};
    std::array<char, name.size()> ret;
    std::ranges::copy(name, ret.begin());
    return ret;
  } else {
    using namespace constexpr_strings;
    constexpr auto rawName = remove_prefixes_t<
      type_name<T>,
      "struct "_constexpr,
      "class "_constexpr>::value;
    if constexpr (subcommand_naming_traits<Traits, T>) {
      return Traits::template normalize_subcommand_name<T, rawName>();
    } else {
      return hyphenate_t<remove_prefixes_or_suffixes_t<
        remove_namespace_t<rawName>::value,
        "Command"_constexpr,
        "command"_constexpr,
        "_"_constexpr>::value>::buffer;
    }
  }
}

}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <
  subcommand T,
  class TParent = value_wrapper_t<typename T::arguments_type>>
struct subcommand_match : TParent {
  using subcommand_type = T;
  using arguments_type = T::arguments_type;

  using TParent::TParent;
  using TParent::operator*;
};

template <
  subcommand T,
  class TParent = value_wrapper_t<incomplete_parse_reason_t>>
struct incomplete_subcommand_parse_reason_t : TParent {
  using subcommand_type = T;

  using TParent::TParent;
  using TParent::operator*;
};

template <subcommand First, subcommand... Rest>
using incomplete_command_parse_reason_t = std::variant<
  help_requested,
  version_requested,
  missing_required_argument,
  invalid_argument_value,
  incomplete_subcommand_parse_reason_t<First>,
  incomplete_subcommand_parse_reason_t<Rest>...>;

}// namespace magic_args::inline public_api

#endif