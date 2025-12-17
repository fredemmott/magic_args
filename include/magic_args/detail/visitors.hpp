// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_VISITORS_HPP
#define MAGIC_ARGS_DETAIL_VISITORS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "concepts.hpp"
#endif

namespace magic_args::detail {

template <class T>
struct visitor_predicate_t {
  constexpr static bool value = true;
};

/** Visit all arguments defined in the return type structure.
 *
 * `ret` is the parsed arguments output structure from a parse.
 *
 * `visitor` is invoked with each argument definition, and a reference to the
 * storage.
 *
 * return 'true' to prevent further execution of the visitor
 *
 * returns true if any invocations of the visitor returned true.
 */
template <parsing_traits Traits>
[[nodiscard]]
bool visit_all_defined_arguments(const auto& visitor, auto& ret) {
  using TRet = std::decay_t<decltype(ret)>;

  auto tuple = tie_struct(ret);
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return (
      visitor(get_argument_definition<TRet, I, Traits>(), get<I>(tuple))
      || ...);
  }(std::make_index_sequence<count_members<TRet>()> {});
}

template <parsing_traits Traits>
[[nodiscard]]
bool visit_options(const auto& visitor, auto& ret) {
  return visit_all_defined_arguments<Traits>(
    [&]<basic_argument TDef, class TValue>(const TDef& def, TValue&& value) {
      if constexpr (basic_option<TDef>) {
        return visitor(def, std::forward<TValue>(value));
      } else {
        return false;
      }
    },
    ret);
}

template <parsing_traits Traits>
[[nodiscard]]
bool visit_positional_arguments(const auto& visitor, auto& ret) {
  return visit_all_defined_arguments<Traits>(
    [&]<basic_argument TDef, class TValue>(const TDef& def, TValue&& value) {
      if constexpr (basic_option<TDef>) {
        return false;
      } else {
        return visitor(def, std::forward<TValue>(value));
      }
    },
    ret);
}

}// namespace magic_args::detail

#endif