// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/gnu_style_parsing_traits.hpp>
#include <magic_args/program_info.hpp>

#include "concepts.hpp"
#include "print.hpp"
#include "to_formattable.hpp"
#endif

#include <cstdio>
#include <filesystem>
#include <string>

namespace magic_args::detail {

template <parsing_traits Traits, basic_argument TArg>
  requires(!basic_option<TArg>)
void show_option_usage(FILE*, const TArg&, const typename TArg::value_type&) {
}

template <basic_option TArg>
struct describe_default_value_t {
  using TValue = TArg::value_type;

  static std::string operator()(const TValue&) {
    return {};
  }

  static std::string operator()(const TValue& value)
    requires detail::formattable<TValue> && std::equality_comparable<TValue>
  {
    if (value == TValue {}) {
      return {};
    }
    return to_string(value);
  }

  static std::string operator()(const TValue& value)
    requires detail::formattable<TValue> && (!std::equality_comparable<TValue>)
    && std::default_initializable<TValue>
  {
    if (const auto ret = to_string(value); ret != to_string(TValue {})) {
      return ret;
    }
    return {};
  }

 private:
  static std::string to_string(const TValue& value)
    requires detail::formattable<TValue>
  {
    return std::format("{}", to_formattable(value));
  }
};

template <
  basic_option TArg,
  same_as_ignoring_cvref<typename TArg::value_type> TValue>
std::string describe_default_value(const TArg&, TValue&& value) {
  return describe_default_value_t<TArg> {}(std::forward<TValue>(value));
}

template <basic_argument TArg>
struct get_argument_help_t {
  static constexpr auto operator()(const TArg& argDef) {
    return argDef.mHelp;
  }
};

template <basic_argument TArg>
constexpr auto get_argument_help(TArg&& argDef) {
  return get_argument_help_t<TArg> {}(std::forward<TArg>(argDef));
}

template <parsing_traits Traits, basic_option TArg>
void show_option_usage(
  FILE* output,
  const TArg& argDef,
  const typename TArg::value_type& initialValue) {
  const auto shortArg = [&argDef] {
    if constexpr (requires {
                    argDef.mShortName;
                    Traits::short_arg_prefix;
                  }) {
      if (!argDef.mShortName.empty()) {
        return std::format(
          "{}{},", Traits::short_arg_prefix, argDef.mShortName);
      }
    }
    return std::string {};
  }();
  const auto longArg = same_as_ignoring_cvref<flag, TArg>
    ? std::format("{}{}", Traits::long_arg_prefix, argDef.mName)
    : std::format("{}{}=VALUE", Traits::long_arg_prefix, argDef.mName);

  const auto header = std::format("  {:3} {}", shortArg, longArg);

  std::vector<std::string> extra;
  if (const auto help = get_argument_help(argDef); !help.empty()) {
    extra.emplace_back(help);
  }
  if (const auto defaultValue = describe_default_value(argDef, initialValue);
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

template <parsing_traits Traits, basic_option T>
void show_positional_argument_usage(FILE*, const T&) {
}

template <parsing_traits Traits, basic_argument T>
  requires(!basic_option<T>)
void show_positional_argument_usage(FILE* output, const T& arg) {
  if (arg.mHelp.empty()) {
    detail::println(output, "      {}", arg.mName);
    return;
  }
  detail::println(output, "      {:25}{}", arg.mName, arg.mHelp);
}

template <class T, parsing_traits Traits = gnu_style_parsing_traits>
void show_usage(
  FILE* output,
  std::string_view argv0,
  const program_info& extraHelp = {}) {
  using namespace detail;
  constexpr auto N = count_members<T>();

  constexpr bool hasOptions = []<std::size_t... I>(std::index_sequence<I...>) {
    return (
      basic_option<decltype(get_argument_definition<T, I, Traits>())> || ...);
  }(std::make_index_sequence<N> {});
  constexpr bool hasPositionalArguments
    = []<std::size_t... I>(std::index_sequence<I...>) {
        return (
          (basic_argument<decltype(get_argument_definition<T, I, Traits>())>
           && !basic_option<decltype(get_argument_definition<T, I, Traits>())>)
          || ...);
      }(std::make_index_sequence<N> {});

  const auto oneLiner = std::format(
    "Usage: {} [OPTIONS...]", std::filesystem::path {argv0}.stem().string());
  if constexpr (!hasPositionalArguments) {
    detail::println(output, "{}", oneLiner);
  } else {
    detail::print(output, "{} [--]", oneLiner);
    []<std::size_t... I>(auto output, std::index_sequence<I...>) {
      (
        [&] {
          const auto arg = get_argument_definition<T, I, Traits>();
          using TArg = std::decay_t<decltype(arg)>;
          if constexpr (!(basic_option<TArg> || std::same_as<TArg, flag>)) {
            auto name = arg.mName;
            for (auto&& c: name) {
              c = static_cast<char>(std::toupper(c));
            }
            if (name.back() == 'S') {
              // Real de-pluralization requires a lookup database; we can't do
              // that, so this seems to be the only practical approach. If
              // it's not good enough for you, specify a
              // `positional_argument<T>` and provide a name.
              name.pop_back();
            }
            if constexpr (vector_like<typename TArg::value_type>) {
              name = std::format("{0} [{0} [...]]", name);
            }
            if (TArg::is_required) {
              detail::print(output, " {}", name);
            } else {
              detail::print(output, " [{}]", name);
            }
          }
        }(),
        ...);
    }(output, std::make_index_sequence<N> {});
    detail::println(output, "");
  }

  if (!extraHelp.mDescription.empty()) {
    detail::println(output, "{}", extraHelp.mDescription);
  }

  if (!extraHelp.mExamples.empty()) {
    detail::print(output, "\nExamples:\n\n");
    for (auto&& example: extraHelp.mExamples) {
      detail::println(output, "  {}", example);
    }
  }

  detail::print(output, "\nOptions:\n\n");
  if (hasOptions) {
    [output]<std::size_t... I>(std::index_sequence<I...>) {
      (show_option_usage<Traits>(
         output,
         get_argument_definition<T, I, Traits>(),
         get<I>(tie_struct(T {}))),
       ...);
    }(std::make_index_sequence<N> {});
    detail::print(output, "\n");
  }

  if constexpr (requires { Traits::short_help_arg; }) {
    show_option_usage<Traits>(
      output,
      flag {Traits::long_help_arg, "show this message", Traits::short_help_arg},
      {});
  } else {
    show_option_usage<Traits>(
      output, flag {Traits::long_help_arg, "show this message"}, {});
  }
  if (!extraHelp.mVersion.empty()) {
    show_option_usage<Traits>(
      output, flag {Traits::version_arg, "print program version"}, {});
  }

  if (hasPositionalArguments) {
    detail::print(output, "\nArguments:\n\n");
    [output]<std::size_t... I>(std::index_sequence<I...>) {
      (show_positional_argument_usage<Traits>(
         output, get_argument_definition<T, I, Traits>()),
       ...);
    }(std::make_index_sequence<N> {});
  }
}
}// namespace magic_args::detail