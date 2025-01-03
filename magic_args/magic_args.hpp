// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <complex>
#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <print>
#include <span>
#include <sstream>

#include "magic_args-reflection.hpp"

namespace magic_args::inline api {
template <class T>
concept basic_argument = requires(T v) {
  typename T::value_type;
  { v.mName } -> std::convertible_to<std::string>;
  { v.mHelp } -> std::convertible_to<std::string>;
};

template <class T>
concept basic_option = requires(T v) {
  typename T::value_type;
  { v.mName } -> std::convertible_to<std::string>;
  { v.mHelp } -> std::convertible_to<std::string>;
  { v.mShortName } -> std::convertible_to<std::string>;
};

template <class T>
concept vector_like = requires { typename T::value_type; }
  && requires(T c, typename T::value_type v) { c.push_back(v); }
  && (!std::same_as<T, std::string>);

template <class T>
struct optional_positional_argument {
  static constexpr bool is_required = false;
  using value_type = T;
  std::string mName;
  std::string mHelp;
  T mValue {};

  optional_positional_argument& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }
  bool operator==(const optional_positional_argument&) const noexcept = default;
};

template <class T>
struct mandatory_positional_argument {
  static constexpr bool is_required = true;
  using value_type = T;
  std::string mName;
  std::string mHelp;
  T mValue {};

  mandatory_positional_argument& operator=(T&& value) {
    mValue = std::move(value);
    return *this;
  }
  operator T() const noexcept {
    return mValue;
  }

  bool operator==(const mandatory_positional_argument&) const noexcept
    = default;
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
  bool operator==(const option&) const noexcept = default;
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

  bool operator==(const flag&) const noexcept = default;
};

static_assert(basic_option<flag>);
static_assert(basic_option<option<std::string>>);

template <class T, std::size_t N, class Traits>
auto infer_argument_definition() {
  // TODO: put the member name -> thing into the traits
  std::string name {detail::Reflection::member_name<T, N>};
  using TValue
    = std::decay_t<decltype(get<N>(detail::Reflection::tie_struct(T {})))>;
  if constexpr (basic_argument<TValue> && !basic_option<TValue>) {
    Traits::normalize_positional_argument_name(name);
  } else {
    Traits::normalize_option_name(name);
  }

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

template <class T, std::size_t N, class Traits>
auto get_argument_definition() {
  using namespace detail::Reflection;

  auto value = get<N>(tie_struct(T {}));
  using TValue = std::decay_t<decltype(value)>;
  if constexpr (basic_argument<TValue>) {
    if (value.mName.empty()) {
      value.mName = infer_argument_definition<T, N, Traits>().mName;
    }
    return value;
  } else {
    return infer_argument_definition<T, N, Traits>();
  }
}

struct gnu_style_parsing_traits {
  static constexpr char long_arg_prefix[] = "--";
  static constexpr char short_arg_prefix[] = "-";
  static constexpr char value_separator[] = "=";
  static constexpr char short_help_arg[] = "?";

  inline static void normalize_option_name(std::string& name);
  inline static void normalize_positional_argument_name(std::string& name);
};

inline void gnu_style_parsing_traits::normalize_option_name(std::string& name) {
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
}

inline void gnu_style_parsing_traits::normalize_positional_argument_name(
  std::string& name) {
  normalize_option_name(name);
  for (auto&& c: name) {
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
      continue;
    }
    if (c == '-') {
      c = '_';
      continue;
    }
  }
}

struct ExtraHelp {
  std::string mDescription;
  std::vector<std::string> mExamples;
  std::string mVersion;
};

template <class Traits, class TArg>
void show_option_usage(FILE*, const TArg&) {
}

template <class Traits, basic_option TArg>
void show_option_usage(FILE* output, const TArg& arg) {
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
    std::println(output, "{}", oneLiner);
  } else {
    std::print(output, "{}", oneLiner);
    []<std::size_t... I>(auto output, std::index_sequence<I...>) {
      (
        [&] {
          const auto arg = get_argument_definition<T, I, Traits>();
          using TArg = std::decay_t<decltype(arg)>;
          if constexpr (!(basic_option<TArg> || std::same_as<TArg, flag>)) {
            auto name = arg.mName;
            for (auto&& c: name) {
              c = std::toupper(c);
            }
            if (name.back() == 'S') {
              // Real de-pluralization requires a lookup database; we can't do
              // that, so this seems to be the only practical approach. If it's
              // not good enough for you, specify a `positional_argument<T>`
              // and provide a name.
              name.pop_back();
            }
            if constexpr (vector_like<typename TArg::value_type>) {
              name = std::format("{0} [{0} [...]]", name);
            }
            if (TArg::is_required) {
              std::print(output, " {}", name);
            } else {
              std::print(output, " [{}]", name);
            }
          }
        }(),
        ...);
    }(output, std::make_index_sequence<N> {});
    std::println(output, "");
  }

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
  if (hasOptions) {
    []<std::size_t... I>(auto output, std::index_sequence<I...>) {
      (show_option_usage<Traits>(
         output, get_argument_definition<T, I, Traits>()),
       ...);
    }(output, std::make_index_sequence<N> {});
    std::println(output, "");
  }

  show_option_usage<Traits>(
    output, flag {"help", "show this message", Traits::short_help_arg});
  if (!extraHelp.mVersion.empty()) {
    show_option_usage<Traits>(
      output, flag {"version", "print program version"});
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
bool option_matches(const T& argDef, std::string_view arg) {
  if (arg == std::format("{}{}", Traits::long_arg_prefix, argDef.mName)) {
    return true;
  }
  if constexpr (requires { Traits::short_arg_prefix; }) {
    if (
      (!argDef.mShortName.empty())
      && arg
        == std::format("{}{}", Traits::short_arg_prefix, argDef.mShortName)) {
      return true;
    }
  }
  return false;
}

enum class incomplete_parse_reason {
  HelpRequested,
  VersionRequested,
  MissingRequiredArgument,
  MissingArgumentValue,
  InvalidArgument,
  InvalidArgumentValue,
};

template <class T>
struct arg_parse_match {
  T mValue;
  std::size_t mConsumed;
};

template <class T>
using arg_parse_result
  = std::optional<std::expected<arg_parse_match<T>, incomplete_parse_reason>>;

template <
  class Traits,
  basic_argument T,
  class V = std::decay_t<typename T::value_type>>
  requires(!basic_option<T>)
arg_parse_result<V> parse_option(
  const T& arg,
  std::span<std::string_view> args) {
  return std::nullopt;
}

template <class T>
void from_string_arg_outer(T& out, std::string_view arg) {
  if constexpr (requires { from_string_arg(out, arg); }) {
    from_string_arg(out, arg);
  } else {
    static_assert(requires(std::stringstream ss, T v) { ss >> v; });
    from_string_arg_fallback(out, arg);
  }
}

template <
  class Traits,
  basic_option T,
  class V = std::decay_t<typename T::value_type>>
arg_parse_result<V> parse_option(
  const T& argDef,
  std::span<std::string_view> args) {
  using enum incomplete_parse_reason;
  if (!option_matches<Traits>(argDef, args.front())) {
    return std::nullopt;
  }
  if (args.size() == 1) {
    return std::unexpected {MissingArgumentValue};
  }

  V ret {};
  from_string_arg_outer(ret, args[1]);
  return {arg_parse_match {ret, 2}};
}

template <class Traits>
arg_parse_result<bool> parse_option(
  const flag& arg,
  std::span<std::string_view> args) {
  if (option_matches<Traits>(arg, args.front())) {
    return {arg_parse_match {true, 1}};
  }
  return std::nullopt;
}

template <class Traits, basic_argument T, class V = typename T::value_type>
  requires(!basic_option<T>)
arg_parse_result<V> parse_positional_argument(
  const T& argDef,
  std::string_view arg0,
  std::span<std::string_view> args,
  FILE* errorStream) {
  if (args.empty()) {
    if constexpr (T::is_required) {
      std::println(
        errorStream, "{}: Missing required argument `{}`", arg0, argDef.mName);
      return std::unexpected {incomplete_parse_reason::MissingArgumentValue};
    }
    return std::nullopt;
  }
  if constexpr (vector_like<V>) {
    V ret {};
    ret.reserve(args.size());
    for (auto&& arg: args) {
      typename V::value_type v {};
      from_string_arg_outer(v, arg);
      ret.push_back(std::move(v));
    }
    return {arg_parse_match {ret, args.size()}};
  } else {
    V ret {};
    from_string_arg_outer(ret, args.front());
    return arg_parse_match {std::move(ret), 1};
  }
}

template <class Traits, basic_option T, class V = typename T::value_type>
arg_parse_result<V> parse_positional_argument(
  const T& argDef,
  std::string_view arg0,
  std::span<std::string_view> args,
  FILE* errorStream) {
  return std::nullopt;
}

template <class T, std::size_t I = 0>
constexpr bool only_last_positional_argument_may_have_multiple_values() {
  using namespace detail::Reflection;
  // Doesn't matter for this check, but we need some traits for
  // get_argument_definition
  using Traits = gnu_style_parsing_traits;

  constexpr auto N = count_members<T>();
  if constexpr (I == N) {
    return true;
  } else if constexpr (vector_like<
                         decltype(get_argument_definition<T, I, Traits>())>) {
    return I == N - 1;
  } else {
    return only_last_positional_argument_may_have_multiple_values<T, I + 1>();
  }
}

template <class T, std::size_t I = 0>
constexpr std::ptrdiff_t last_mandatory_positional_argument() {
  using namespace detail::Reflection;
  using Traits = gnu_style_parsing_traits;

  constexpr auto N = count_members<T>();
  if constexpr (I == N) {
    return -1;
  } else {
    const auto recurse = last_mandatory_positional_argument<T, I + 1>();
    using TArg
      = std::decay_t<decltype(get_argument_definition<T, I, Traits>())>;
    if constexpr (requires { TArg::is_required; }) {
      if (recurse == -1 && TArg::is_required) {
        return I;
      }
    }
    return recurse;
  }
};

template <class T, std::size_t I = 0>
constexpr std::ptrdiff_t first_optional_positional_argument() {
  using namespace detail::Reflection;
  using Traits = gnu_style_parsing_traits;
  constexpr auto N = count_members<T>();
  if constexpr (I == N) {
    return -1;
  } else {
    using TArg
      = std::decay_t<decltype(get_argument_definition<T, I, Traits>())>;
    if constexpr (requires { TArg::is_required; }) {
      if (!TArg::is_required) {
        return I;
      }
    }
    return first_optional_positional_argument<T, I + 1>();
  }
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
  const auto versionArg = std::format("{}version", Traits::long_arg_prefix);

  for (auto&& arg: args) {
    if (arg == "--") {
      break;
    }
    if (arg == longHelp || (arg == shortHelp && !shortHelp.empty())) {
      show_usage<T, Traits>(outputStream, args.front(), help);
      return std::unexpected {incomplete_parse_reason::HelpRequested};
    }
    if (arg == versionArg && !help.mVersion.empty()) {
      std::println(outputStream, "{}", help.mVersion);
      return std::unexpected {incomplete_parse_reason::VersionRequested};
    }
  }

  const auto arg0 = std::filesystem::path {args.front()}.stem().string();
  using namespace detail::Reflection;
  T ret {};
  auto tuple = tie_struct(ret);

  constexpr auto N = count_members<T>();
  std::vector<std::string_view> positionalArgs;

  // Handle options
  std::optional<incomplete_parse_reason> failure;
  for (std::size_t i = 1; i < args.size();) {
    const auto arg = args[i];
    if (arg == "--") {
      std::ranges::copy(
        args.subspan(i + 1), std::back_inserter(positionalArgs));
      break;
    }

    const auto matchedOption
      = [&]<std::size_t... I>(std::index_sequence<I...>) {
          // returns bool: matched option
          return ([&] {
            const auto def = get_argument_definition<T, I, Traits>();
            auto result = parse_option<Traits>(def, args.subspan(i));
            if (!result) {
              return false;
            }
            if (!result->has_value()) {
              failure = result->error();
              return true;
            }
            get<I>(tuple) = std::move((*result)->mValue);
            i += (*result)->mConsumed;
            return true;
          }() || ...);
        }(std::make_index_sequence<N> {});

    if (failure) {
      std::println(errorStream, "");
      show_usage<T, Traits>(errorStream, args.front(), help);
      return std::unexpected {failure.value()};
    }
    if (matchedOption) {
      continue;
    }

    // Store positional parameters for later
    [&]<std::size_t... I>(std::index_sequence<I...>) {
    }(std::make_index_sequence<N> {});

    if (arg.starts_with(Traits::long_arg_prefix)) {
      std::print(errorStream, "{}: Unrecognized option: {}\n\n", arg0, arg);
      show_usage<T, Traits>(errorStream, args.front(), help);
      return std::unexpected {incomplete_parse_reason::InvalidArgument};
    }
    if constexpr (requires { Traits::short_arg_prefix; }) {
      // TODO: handle -abc where `a`, `b`, and `c` are all flags

      // The short prefixes have other meanings, e.g.:
      //
      // GNU, Powershell: `-` often means 'stdout'
      // Classic MS: '/' can mean 'root of the filesystem
      if (
        arg.starts_with(Traits::short_arg_prefix)
        && arg != Traits::short_arg_prefix) {
        std::print(errorStream, "{}: Unrecognized option: {}\n\n", arg0, arg);
        show_usage<T, Traits>(errorStream, args.front(), help);
        return std::unexpected {incomplete_parse_reason::InvalidArgument};
      }
    }

    positionalArgs.emplace_back(arg);
    ++i;
  }

  // Handle positional args
  static_assert(only_last_positional_argument_may_have_multiple_values<T>());
  static_assert(
    first_optional_positional_argument<T>()
    >= last_mandatory_positional_argument<T>());
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    (void)([&] {
      // returns bool: continue
      const auto def = get_argument_definition<T, I, Traits>();
      auto result = parse_positional_argument<Traits>(
        def, arg0, positionalArgs, errorStream);
      if (!result) {
        return true;
      }
      if (!result->has_value()) {
        failure = result->error();
        return false;
      }
      get<I>(tuple) = std::move((*result)->mValue);
      positionalArgs.erase(
        positionalArgs.begin(), positionalArgs.begin() + (*result)->mConsumed);
      return true;
    }() && ...);
  }(std::make_index_sequence<N> {});
  if (failure) {
    std::println(errorStream, "");
    show_usage<T, Traits>(errorStream, args.front(), help);
    return std::unexpected {failure.value()};
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
