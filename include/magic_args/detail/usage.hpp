// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_USAGE_HPP
#define MAGIC_ARGS_DETAIL_USAGE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/gnu_style_parsing_traits.hpp>

#include "concepts.hpp"
#include "get_argument_definition.hpp"
#include "parse.hpp"
#include "parsing_traits_for_args.hpp"
#include "print.hpp"
#include "to_formattable.hpp"
#endif

#include <cstdio>
#include <filesystem>
#include <string>

namespace magic_args::detail {

template <class T>
struct generate_argument_help_t {
  static std::string_view operator()() {
    return {};
  }
};

template <class TArgs, std::size_t I>
static auto get_argument_help_by_index() {
  using member_type = member_type_by_index<TArgs, I>;
  if constexpr (!basic_argument<member_type>) {
    return generate_argument_help_t<member_type> {}();
  } else {
    constexpr auto value = std::get<I>(tie_struct(TArgs {})).mHelp;
    if constexpr (!value.empty()) {
      return value;
    } else {
      return generate_argument_help_t<typename member_type::value_type> {}();
    }
  }
}

template <
  class TArgs,
  std::size_t I,
  parsing_traits Traits,
  class TArgDef = argument_definition_t<TArgs, I, Traits>>
  requires(!static_basic_option<TArgDef>)
void show_option_usage(FILE*) {
}

template <static_basic_option TArgDef>
struct describe_default_value_t {
  using value_type = TArgDef::value_type;

  static std::string operator()(const value_type&) {
    return {};
  }

  static std::string operator()(const value_type& value)
    requires detail::formattable<value_type>
    && std::equality_comparable<value_type>
  {
    if (value == value_type {}) {
      return {};
    }
    return to_string(value);
  }

  static std::string operator()(const value_type& value)
    requires detail::formattable<value_type>
    && (!std::equality_comparable<value_type>)
    && std::default_initializable<value_type>
  {
    if (const auto ret = to_string(value); ret != to_string(value_type {})) {
      return ret;
    }
    return {};
  }

 private:
  static std::string to_string(const value_type& value)
    requires detail::formattable<value_type>
  {
    return std::format("{}", to_formattable(value));
  }
};

template <
  class TArgs,
  std::size_t I,
  parsing_traits Traits,
  class TArgDef = argument_definition_t<TArgs, I, Traits>>
void show_option_usage(FILE* output) {
  const auto shortArg = [] {
    constexpr auto ShortName
      = argument_definition_t<TArgs, I, Traits>::short_name;
    if constexpr (!ShortName.empty()) {
      return std::format(
        "{}{},",
        std::string {Traits::short_arg_prefix},
        std::string {ShortName});
    } else {
      return std::string {};
    }
  }();
  const auto longArg = [&] {
    if constexpr (TArgDef::behavior == Behavior::Flag) {
      static_assert(
        static_flag<TArgDef>,
        "Have a static argument definition with Flag behavior, but not a "
        "static flag definition");

      constexpr auto setTrue = TArgDef::name;
      constexpr auto setFalse = TArgDef::negated_flag_name;

      if constexpr (!setFalse.empty()) {
        if (setFalse.ends_with(setTrue)) {
          return std::format(
            "{}[{}]{}",
            Traits::long_arg_prefix,
            setFalse.substr(0, setFalse.size() - setTrue.size()),
            setTrue);
        } else if (setFalse.starts_with(setTrue)) {
          return std::format(
            "{}{}[{}]",
            Traits::long_arg_prefix,
            setTrue,
            setFalse.substr(setTrue.size()),
            setTrue);
        } else {
          return std::format("{}{}", Traits::long_arg_prefix, setTrue);
        }
      } else {
        return std::format("{}{}", Traits::long_arg_prefix, setTrue);
      }
    } else if constexpr (TArgDef::behavior == Behavior::CountedFlag) {
      return std::format(
        "{}{}[{}VALUE]",
        std::string_view {Traits::long_arg_prefix},
        std::string_view {TArgDef::name},
        std::string_view {Traits::value_separator});
    } else {
      return std::format(
        "{}{}{}VALUE",
        std::string_view {Traits::long_arg_prefix},
        std::string_view {TArgDef::name},
        std::string_view {Traits::value_separator});
    }
  }();

  const auto header = std::format("  {:3} {}", shortArg, longArg);

  std::vector<std::string> extra;
  if (const auto help = get_argument_help_by_index<TArgs, I>(); !help.empty()) {
    extra.emplace_back(help);
  }

  if (const auto defaultValue
      = describe_default_value_t<TArgDef> {}(TArgDef::default_value());
      !defaultValue.empty()) {
    extra.emplace_back(std::format("(default: {})", defaultValue));
  }

  if (extra.empty()) {
    detail::println(output, "{}", header);
    return;
  }

  std::string_view prefix;
  if (header.size() <= 30) {
    prefix = header;
  } else {
    detail::println(output, "{}", header);
  }

  for (auto&& line: extra) {
    detail::println(output, "{:30} {}", prefix, line);
    prefix = {};
  }
}

template <
  class TArgs,
  std::size_t I,
  parsing_traits Traits,
  class TArgDef = argument_definition_t<TArgs, I, Traits>>
void show_positional_argument_usage(FILE*) {
}

template <
  class TArgs,
  std::size_t I,
  parsing_traits Traits,
  static_basic_positional_argument TArgDef
  = argument_definition_t<TArgs, I, Traits>>
void show_positional_argument_usage(FILE* output) {
  const auto help = get_argument_help_by_index<TArgs, I>();
  if (help.empty()) {
    detail::println(output, "      {}", std::string_view {TArgDef::name});
    return;
  }
  detail::println(
    output, "      {:25}{}", std::string_view {TArgDef::name}, help);
}

template <class T, std::size_t I, parsing_traits Traits>
void append_positional_name(FILE* output) {
  using TArgDef = argument_definition_t<T, I, Traits>;
  if constexpr (is_positional_argument(TArgDef::behavior)) {
    std::string name {TArgDef::name};
    if (toupper(name.back()) == 'S') {
      // Real de-pluralization requires a lookup database; we can't do
      // that, so this seems to be the only practical approach. If
      // it's not good enough for you, specify a
      // `positional_argument<T>` and provide a name.
      name.pop_back();
    }
    if constexpr (vector_like<typename TArgDef::value_type>) {
      name = std::format("{0} [{0} [...]]", name);
    }
    if (is_required(TArgDef::behavior)) {
      detail::print(output, " {}", name);
    } else {
      detail::print(output, " [{}]", name);
    }
  }
}

template <parsing_traits Traits, class T>
void show_usage(FILE* output, argv_range auto&& argv) {
  using namespace detail;
  constexpr auto N = count_members<T>();

  constexpr bool hasOptions = []<std::size_t... I>(std::index_sequence<I...>) {
    return (is_option(argument_definition_t<T, I, Traits>::behavior) || ...);
  }(std::make_index_sequence<N> {});
  constexpr bool hasPositionalArguments
    = []<std::size_t... I>(std::index_sequence<I...>) {
        return (
          is_positional_argument(argument_definition_t<T, I, Traits>::behavior)
          || ...);
      }(std::make_index_sequence<N> {});

  const auto oneLiner = std::format(
    "Usage: {} [OPTIONS...]",
    detail::get_prefix_for_user_messages<Traits>(argv));
  if constexpr (!hasPositionalArguments) {
    detail::println(output, "{}", oneLiner);
  } else {
    detail::print(output, "{} [--]", oneLiner);
    []<std::size_t... I>(FILE* output, std::index_sequence<I...>) {
      (append_positional_name<T, I, Traits>(output), ...);
    }(output, std::make_index_sequence<N> {});
    detail::println(output, "");
  }

  if constexpr (has_description<T>) {
    detail::println(output, "{}", std::string_view {T::description});
  }

  if constexpr (has_examples<T>) {
    detail::print(output, "\nExamples:\n\n");
    for (auto&& example: T::examples) {
      detail::println(output, "  {}", std::string_view {example});
    }
  }

  detail::print(output, "\nOptions:\n\n");
  if (hasOptions) {
    [output]<std::size_t... I>(std::index_sequence<I...>) {
      (show_option_usage<T, I, Traits>(output), ...);
    }(std::make_index_sequence<N> {});
    detail::print(output, "\n");
  }

  const auto longHelp
    = std::format("{}{}", Traits::long_arg_prefix, Traits::long_help_arg);
  const auto shortHelp = std::string_view {Traits::short_help_arg}.empty()
    ? std::string {}
    : std::format("{}{}", Traits::short_arg_prefix, Traits::short_help_arg);
  const auto version = has_version<T>
    ? std::format("{}{}", Traits::long_arg_prefix, Traits::version_arg)
    : std::string {};

  if (shortHelp.empty()) {
    std::println(output, "        {:24} show this message", longHelp);
  } else {
    std::println(
      output, "  {:2}, {:24} show this message", shortHelp, longHelp);
  }
  if constexpr (has_version<T>) {
    std::println(output, "      {:24} print program version", version);
  }

  if (hasPositionalArguments) {
    detail::print(output, "\nArguments:\n\n");
    [output]<std::size_t... I>(std::index_sequence<I...>) {
      (show_positional_argument_usage<T, I, Traits>(output), ...);
    }(std::make_index_sequence<N> {});
  }
}
}// namespace magic_args::detail

#endif