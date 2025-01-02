// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <print>
#include <span>

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
  T mValue {};

  option& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }
};

struct flag final {
  using value_type = bool;
  std::string mName;
  std::string mHelp;
  std::string mShortName;
  bool mValue {false};

  flag& operator=(bool value) {
    mValue = value;
    return *this;
  }
  operator bool() const noexcept {
    return mValue;
  }
};

static_assert(basic_argument<flag>);
static_assert(basic_argument<option<std::string>>);

template <class T, std::size_t N>
auto infer_argument_definition() {
  // TODO: put the member name -> thing into the traits
  std::string name {detail::Reflection::member_name<T, N>};
  if (
    name.starts_with('m') && name.size() > 1 && name[1] >= 'A'
    && name[1] <= 'Z') {
    name = name.substr(1);
  }
  if (name.starts_with('_')) {
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
void show_arg_usage(FILE* output, const TArg& arg) {
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
  FILE* output,
  std::string_view argv0,
  const ExtraHelp& extraHelp = {}) {
  using namespace detail::Reflection;

  std::println(
    output,
    "Usage: {} [OPTIONS...]",
    std::filesystem::path {argv0}.stem().string());
  if (!extraHelp.mDescription.empty()) {
    std::println(output, "{}", extraHelp.mDescription);
  }

  if (!extraHelp.mExamples.empty()) {
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
    show_arg_usage<Traits>(output, flag {"version", "print program version"});
  }
}

template <class T>
struct arg_parse_match {
  T mValue;
  std::size_t mConsumed;
};

template <class T>
using arg_parse_result = std::optional<arg_parse_match<T>>;

template <class Traits>
auto parse_arg(const basic_argument auto& arg, std::span<std::string_view>)
  -> arg_parse_result<typename std::decay_t<decltype(arg)>::value_type> {
  return std::nullopt;
}

template <class Traits>
auto parse_arg(const flag& arg, std::span<std::string_view> args)
  -> arg_parse_result<bool> {
  if (args.front() == std::format("{}{}", Traits::long_arg_prefix, arg.mName)) {
    return {{true, 1}};
  }
  if constexpr (Traits::short_arg_prefix) {
    if (
      (!arg.mShortName.empty())
      && args.front()
        == std::format("{}{}", Traits::short_arg_prefix, arg.mShortName)) {
      return {{true, 1}};
    }
  }
  return std::nullopt;
}

enum class no_arguments_reason {
  HelpRequested,
  VersionRequested,
  MissingRequiredArgument,
  InvalidArgument,
};

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, no_arguments_reason> parse(
  std::span<std::string_view> args,
  const ExtraHelp& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto longHelp = std::format("{}help", Traits::long_arg_prefix);
  const auto shortHelp = [] {
    if constexpr (requires {
                    Traits::short_help_arg;
                    Traits::short_arg_prefix;
                  }) {
      return std::format(
        "{}{}", Traits::short_arg_prefix, Traits::short_help_arg);
    } else {
      return std::string {};
    }
  }();

  for (auto&& arg: args) {
    if (arg == "--") {
      break;
    }
    if (arg == longHelp || (arg == shortHelp && !shortHelp.empty())) {
      show_usage<T, Traits>(outputStream, args.front(), help);
      return std::unexpected {no_arguments_reason::HelpRequested};
    }
  }

  using namespace detail::Reflection;
  T ret {};
  auto tuple = tie_struct(ret);

  constexpr auto N = count_members<T>();
  for (std::size_t i = 0; i < args.size(); ++i) {
    bool matched = false;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (
        [&] {
          const auto def = get_argument_definition<T, I>();
          if (auto result = parse_arg<Traits>(def, args.subspan(i))) {
            matched = true;
            get<I>(tie_struct(ret)) = std::move(result->mValue);
            i += result->mConsumed - 1;
          }
        }(),
        ...);
    }(std::make_index_sequence<N> {});
    if (matched) {
      continue;
    }
  }
  return ret;
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, no_arguments_reason> parse(
  int argc,
  char** argv,
  const ExtraHelp& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (auto&& arg: std::span {argv, static_cast<std::size_t>(argc)}) {
    args.emplace_back(arg);
  }
  return parse<T, Traits>(std::span {args}, help, outputStream, errorStream);
}

template <class T>
auto argument_value(const T& arg) {
  if constexpr (basic_argument<T>) {
    return arg.mValue;
  } else {
    return arg;
  }
}

template <class T>
void dump(const T& args, FILE* output = stdout) {
  using namespace detail::Reflection;
  const auto tuple = tie_struct(args);

  []<std::size_t... I>(
    const auto& args, FILE* output, std::index_sequence<I...>) {
    (std::println(
       output, "{:29} `{}`", member_name<T, I>, argument_value(get<I>(args))),
     ...);
  }(tuple,
    output,
    std::make_index_sequence<std::tuple_size_v<decltype(tuple)>> {});
}

}// namespace magic_args::inline api
