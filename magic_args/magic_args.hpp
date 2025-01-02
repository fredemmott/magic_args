// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <concepts>
#include <filesystem>
#include <format>
#include <print>
#include <ranges>

#include "magic_args-reflection.hpp"

namespace magic_args::inline api {
template <class T>
concept basic_argument = requires(T v) {
  typename T::value_type;
  { v.mName } -> std::convertible_to<std::string>;
  { v.mHelp } -> std::convertible_to<std::string>;
  { v.mShortName } -> std::convertible_to<std::string>;
};

template <class T>
struct option final {
  using value_type = T;
  std::string mName;
  std::string mHelp;
  std::string mShortName;
};

struct flag final {
  using value_type = bool;
  std::string mName;
  std::string mHelp;
  std::string mShortName;
};

static_assert(basic_argument<flag>);
static_assert(basic_argument<option<std::string>>);

template <class T, std::size_t N>
auto infer_argument_definition() {
  // TODO: put the member name -> thing into the traits
  std::string name {detail::Reflection::member_name<T, N>};
  if (name.starts_with("m")) {
    name = name.substr(1);
  }
  if (name.starts_with("_")) {
    name = name.substr(1);
  }
  if (name[0] >= 'A' && name[0] <= 'Z') {
    name[0] -= 'A' - 'a';
  }

  for (std::size_t i = 1; i < name.size(); ++i) {
    if (name[i] == '_') {
      name[i] = '-';
      continue;
    }
    if (name[i] >= 'A' && name[i] <= 'Z') {
      name[i] -= 'A' - 'a';
      name.insert(i, 1, '-');
      ++i;
      continue;
    }
  }

  using TValue
    = std::decay_t<decltype(get<N>(detail::Reflection::tie_struct(T {})))>;
  if constexpr (std::same_as<TValue, bool>) {
    return flag {
      .mName = name,
    };
  } else {
    return option<TValue> {
      .mName = name,
    };
  }
}

template <class T, std::size_t N>
auto get_argument_definition() {
  using namespace detail::Reflection;

  auto value = get<N>(tie_struct(T {}));
  using TValue = std::decay_t<decltype(value)>;
  if constexpr (basic_argument<TValue>) {
    if (value.mName.empty()) {
      value.mName = infer_argument_definition<T, N>().mName;
    }
    return value;
  } else {
    return infer_argument_definition<T, N>();
  }
}

struct gnu_style_parsing_traits {
  static constexpr auto long_arg_prefix = "--";
  static constexpr auto short_arg_prefix = "-";
  static constexpr auto value_separator = "=";
  static constexpr auto short_help_arg = "?";
};

struct ExtraHelp {
  std::string mDescription;
  std::vector<std::string> mExamples;
  std::string mVersion;
};

template <class Traits, basic_argument TArg>
void show_arg_usage(auto output, const TArg& arg) {
  const auto shortArg = [&arg] {
    if constexpr (requires { arg.mShortName; }) {
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
    }
    return std::format("{}{}VALUE", name, Traits::value_separator);
  }();

  const auto params = std::format("  {:3} {}", shortArg, longArg);
  if (arg.mHelp.empty()) {
    std::println(output, "{}", params);
    return;
  }

  if (params.size() < 30) {
    std::println(output, "{:30} {}", params, arg.mHelp);
    return;
  }
  std::println(output, "{}\n{:30}{}", params, "", arg.mHelp);
}

template <class T, class Traits = gnu_style_parsing_traits>
void show_usage(
  auto output,
  const char* argv0,
  const ExtraHelp& extraHelp = {}) {
  using namespace detail::Reflection;

  std::println(
    output,
    "Usage: {} [OPTIONS...]",
    std::filesystem::path {argv0}.stem().string());
  if (!extraHelp.mDescription.empty()) {
    std::println(output, "{}", extraHelp.mDescription);
  }

  if (!extraHelp.mExamples .empty()) {
    std::print(output, "\nExamples:\n\n");
    for (auto&& example: extraHelp.mExamples) {
      std::println(output, "  {}", example);
    }
  }

  std::print(output, "\nOptions:\n\n");

  constexpr auto N = count_members<T>();
  []<std::size_t... I>(auto output, std::index_sequence<I...>) {
    (show_arg_usage<Traits>(output, get_argument_definition<T, I>()), ...);
  }(output, std::make_index_sequence<N> {});

  std::println(output, "");
  show_arg_usage<Traits>(
    output, flag {"help", "show this message", Traits::short_help_arg});
  if (!extraHelp.mVersion.empty()) {
    show_arg_usage<Traits>(output, flag { "version", "print program version"});
  }
}
}// namespace magic_args::inline api
