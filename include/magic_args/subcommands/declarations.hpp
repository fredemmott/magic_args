// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_DECLARATIONS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_DECLARATIONS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/incomplete_parse_reason.hpp>
#include <magic_args/value_wrapper_t.hpp>
#endif

#include <concepts>
#include <string_view>
#include <type_traits>

namespace magic_args::inline public_api {

template <class T>
concept subcommand = requires(T v) {
  typename T::arguments_type;
  requires std::default_initializable<typename T::arguments_type>;
  { T::name } -> std::convertible_to<std::string_view>;
};

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

  static constexpr std::string_view subcommand_name {T::name};
};

template <subcommand First, subcommand... Rest>
using incomplete_command_parse_reason_t = std::variant<
  help_requested,
  version_requested,
  missing_required_argument,
  invalid_argument_value,
  incomplete_subcommand_parse_reason_t<First>,
  incomplete_subcommand_parse_reason_t<Rest>...>;

template <class T>
concept root_command_traits = (!subcommand<T>)
  && (detail::has_version<T> || detail::has_description<T>
      || detail::has_examples<T> || has_parsing_traits<T> || parsing_traits<T>);

}// namespace magic_args::inline public_api

#endif