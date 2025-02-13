// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/gnu_style_parsing_traits.hpp>
#include <magic_args/program_info.hpp>

#include "concepts.hpp"
#include "print.hpp"
#endif

#include <cstdio>
#include <filesystem>
#include <string>

namespace magic_args::detail {

template <class Traits, class TArg>
void show_option_usage(FILE*, const TArg&) {
}

template <class Traits, basic_option TArg>
void show_option_usage(FILE* output, const TArg& arg) {
  const auto shortArg = [&arg] {
    if constexpr (requires {
                    arg.mShortName;
                    Traits::short_arg_prefix;
                  }) {
      if (!arg.mShortName.empty()) {
        return std::format("{}{},", Traits::short_arg_prefix, arg.mShortName);
      }
    }
    return std::string {};
  }();

  const auto longArg = [&arg] {
    const std::string name
      = std::format("{}{}", Traits::long_arg_prefix, arg.mName);
    if constexpr (std::same_as<std::decay_t<decltype(arg)>, flag>) {
      return name;
    } else {
      return std::format("{}{}VALUE", name, Traits::value_separator);
    }
  }();

  const auto params = std::format("  {:3} {}", shortArg, longArg);
  if (arg.mHelp.empty()) {
    detail::println(output, "{}", params);
    return;
  }

  if (params.size() < 30) {
    detail::println(output, "{:30} {}", params, arg.mHelp);
    return;
  }
  detail::println(output, "{}\n{:31}{}", params, "", arg.mHelp);
}

template <class Traits, basic_option T>
void show_positional_argument_usage(FILE*, const T&) {
}

template <class Traits, basic_argument T>
  requires(!basic_option<T>)
void show_positional_argument_usage(FILE* output, const T& arg) {
  if (arg.mHelp.empty()) {
    detail::println(output, "      {}", arg.mName);
    return;
  }
  detail::println(output, "      {:25}{}", arg.mName, arg.mHelp);
}

template <class T, class Traits = gnu_style_parsing_traits>
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
         output, get_argument_definition<T, I, Traits>()),
       ...);
    }(std::make_index_sequence<N> {});
    detail::print(output, "\n");
  }

  if constexpr (requires { Traits::short_help_arg; }) {
    show_option_usage<Traits>(
      output,
      flag {
        Traits::long_help_arg, "show this message", Traits::short_help_arg});
  } else {
    show_option_usage<Traits>(
      output, flag {Traits::long_help_arg, "show this message"});
  }
  if (!extraHelp.mVersion.empty()) {
    show_option_usage<Traits>(
      output, flag {Traits::version_arg, "print program version"});
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