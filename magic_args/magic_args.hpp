// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <complex>
#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <print>
#include <span>
#include <sstream>
#include <variant>

#include "magic_args-reflection.hpp"

namespace magic_args::inline api {
template <class T>
concept basic_option = requires(T v) {
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

static_assert(basic_option<flag>);
static_assert(basic_option<option<std::string>>);

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
  if constexpr (basic_option<TValue>) {
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

template <class Traits, basic_option TArg>
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

inline void from_string_arg(std::string& v, std::string_view arg) {
  v = std::string {arg};
}

template <class T>
void from_string_arg_fallback(T& v, std::string_view arg) {
  // TODO (C++26): we should be able to directly use the string_view
  std::stringstream ss {std::string {arg}};
  ss >> v;
}

template <class Traits, basic_option T>
[[nodiscard]]
bool arg_matches(const T& argDef, std::string_view arg) {
  if (arg == std::format("{}{}", Traits::long_arg_prefix, argDef.mName)) {
    return true;
  }
  if constexpr (Traits::short_arg_prefix) {
    if (
      (!argDef.mShortName.empty())
      && arg
        == std::format("{}{}", Traits::short_arg_prefix, argDef.mShortName)) {
      return true;
    }
  }
  return false;
}

enum class arg_parse_failure_reason {
  MissingValue,
  InvalidValue,
};

template <class T>
struct arg_parse_match {
  T mValue;
  std::size_t mConsumed;
};

template <class T>
using arg_parse_result
  = std::optional<std::expected<arg_parse_match<T>, arg_parse_failure_reason>>;

template <
  class Traits,
  basic_option T,
  class V = std::decay_t<typename T::value_type>>
arg_parse_result<V> parse_arg(const T& arg, std::span<std::string_view> args) {
  using enum arg_parse_failure_reason;
  if (!arg_matches<Traits>(arg, args.front())) {
    return std::nullopt;
  }
  if (args.size() == 1) {
    return std::unexpected {MissingValue};
  }

  V ret {};
  // TODO (c++26): args.at(1)
  const auto value = args[1];
  if constexpr (requires { from_string_arg(ret, ret.front()); }) {
    from_string_arg(ret, value);
  } else {
    static_assert(requires(std::stringstream ss, V v) { ss >> v; });
    from_string_arg_fallback(ret, value);
  }
  return {arg_parse_match {ret, 2}};
}

template <class Traits>
arg_parse_result<bool> parse_arg(
  const flag& arg,
  std::span<std::string_view> args) {
  if (arg_matches<Traits>(arg, args.front())) {
    return {arg_parse_match {true, 1}};
  }
  return std::nullopt;
}

enum class incomplete_parse_reason {
  HelpRequested,
  VersionRequested,
  MissingRequiredArgument,
  MissingArgumentValue,
  InvalidArgument,
  InvalidArgumentValue,
  UnrecognizedArgument,
};

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
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
      return std::unexpected {incomplete_parse_reason::HelpRequested};
    }
  }

  using namespace detail::Reflection;
  T ret {};
  auto tuple = tie_struct(ret);

  constexpr auto N = count_members<T>();
  std::vector<std::string_view> positional;

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--") {
      if (args.size() > i + 1) {
        positional.reserve(positional.size() + args.size() - (i + 1));
        std::ranges::copy(args.subspan(i + 1), std::back_inserter(positional));
      }
      break;
    }

    std::optional<incomplete_parse_reason> failure;
    const auto matched = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return ([&] {
        const auto def = get_argument_definition<T, I>();
        auto result = parse_arg<Traits>(def, args.subspan(i));
        if (!result) {
          return false;
        }
        if (!result->has_value()) {
          using enum arg_parse_failure_reason;
          switch (result->error()) {
            case MissingValue:
              failure = incomplete_parse_reason::MissingArgumentValue;
              break;
            case InvalidValue:
              failure = incomplete_parse_reason::InvalidArgumentValue;
              break;
          }
          return true;
        }
        get<I>(tie_struct(ret)) = std::move((*result)->mValue);
        i += (*result)->mConsumed - 1;
        return true;
      }() || ...);
    }(std::make_index_sequence<N> {});
    if (matched) {
      continue;
    }

    const auto arg = args[i];
    if (arg.starts_with(Traits::long_arg_prefix)) {
      std::print(errorStream, "Unrecognized argument: {}\n\n", arg);
      show_usage<T, Traits>(errorStream, args.front(), help);
      return std::unexpected {incomplete_parse_reason::UnrecognizedArgument};
    }
    if constexpr (requires { Traits::short_arg_prefix; }) {
      // TODO: handle -abc where `a`, `b`, and `c` are all flags
      if (arg.starts_with(Traits::short_arg_prefix) && arg != "-") {
        // "-" can mean stdout
        std::print(errorStream, "Unrecognized argument: {}\n\n", arg);
        show_usage<T, Traits>(errorStream, args.front(), help);
        return std::unexpected {incomplete_parse_reason::UnrecognizedArgument};
      }
    }
  }
  return ret;
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
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
  if constexpr (basic_option<T>) {
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
