// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/parse.hpp"
#include "detail/print_incomplete_parse_reason.hpp"
#include "gnu_style_parsing_traits.hpp"
#endif

#include <concepts>
#include <expected>
#include <string_view>

namespace magic_args {
template <class TValue>
class value_wrapper_t {
 public:
  using value_type = TValue;

  value_wrapper_t() = delete;
  constexpr explicit value_wrapper_t(TValue value) : mValue {std::move(value)} {
  }

  template <class Self>
  decltype(auto) value(this Self&& self) noexcept {
    return std::forward<Self>(self).mValue;
  }

  template <class Self>
  constexpr decltype(auto) operator*(this Self&& self) noexcept {
    return std::forward<Self>(self).mValue;
  }

 private:
  TValue mValue;
};
}// namespace magic_args

namespace magic_args::inline public_api {

template <class T>
concept subcommand = requires(T v) {
  typename T::arguments_type;
  requires std::default_initializable<typename T::arguments_type>;
  { T::name } -> std::convertible_to<std::string_view>;
};

using incomplete_command_parse_reason_t = std::variant<
  help_requested,
  version_requested,
  missing_required_argument,
  invalid_argument_value>;

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
}// namespace magic_args::inline public_api
namespace magic_args::detail {
template <
  parsing_traits Traits,
  subcommand First,
  subcommand... Rest,
  class TExpected>
void parse_subcommands_silent_impl(
  std::optional<TExpected>& result,
  std::string_view command,
  argv_range auto&& argv,
  const program_info& help) {
  if (std::string_view {First::name} != command) {
    if constexpr (sizeof...(Rest) > 0) {
      parse_subcommands_silent_impl<Traits, Rest...>(
        result, command, argv, help);
    }
    return;
  }

  // Skip argv[0] and argv[1], instead of just argv[0]
  struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};
  auto subcommandResult
    = parse_silent<typename First::arguments_type, InnerTraits>(argv, help);
  if (subcommandResult) [[likely]] {
    result.emplace(
      subcommand_match<First>(std::move(subcommandResult).value()));
  } else {
    result.emplace(
      std::unexpect,
      incomplete_subcommand_parse_reason_t<First>(
        std::move(subcommandResult).error()));
  }
}
}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <
  parsing_traits Traits,
  subcommand First,
  subcommand... Rest,
  class TSuccess
  = std::variant<subcommand_match<First>, subcommand_match<Rest>...>,
  class TIncomplete = std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected parse_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  if (argv.size() < 2) {
    return std::unexpected {missing_required_argument {
      .mSource = {.mName = "COMMAND"},
    }};
  }
  using CommonArguments = detail::common_arguments_t<Traits>;

  const std::string_view command {*(std::ranges::begin(argv) + 1)};
  if (
    command == CommonArguments::long_help
    || command == CommonArguments::short_help || command == "help") {
    return std::unexpected {help_requested {}};
  }
  if (command == CommonArguments::version && !help.mVersion.empty()) {
    return std::unexpected {version_requested {}};
  }

  std::optional<TExpected> result;
  detail::parse_subcommands_silent_impl<Traits, First, Rest...>(
    result, command, argv, help);
  if (result) [[likely]] {
    return std::move(result).value();
  }

  return std::unexpected {invalid_argument_value {
        .mSource = {
          .mArgvSlice = std::vector { std::string { command } },
          .mName = "COMMAND",
          .mValue = std::string { command },
        },
      }};
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& help = {}) {
  return parse_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<decltype(argv)>(argv), help);
}

template <class... Args>
auto parse_subcommands_silent(
  const int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands_silent<Args...>(
    std::views::counted(argv, argc), help, outputStream, errorStream);
}
}// namespace magic_args::inline public_api

namespace magic_args::detail {

template <parsing_traits Traits, subcommand... Ts>
void show_subcommand_usage(
  const program_info& info,
  argv_range auto&& argv,
  FILE* stream) {
  using CommonArguments = common_arguments_t<Traits>;
  detail::println(
    stream,
    "Usage: {} COMMAND [OPTIONS...]\n",
    get_prefix_for_user_messages<Traits>(argv));
  detail::println(stream, "Commands:\n");

  (
    [stream]<class T>(std::type_identity<T>) {
      // TODO: show help
      detail::println(stream, "      {}", T::name);
    }(std::type_identity<Ts> {}),
    ...);

  detail::println(
    stream,
    "\n  {:2}, {:24} show this message",
    *CommonArguments::short_help,
    *CommonArguments::long_help);

  if (!info.mVersion.empty()) {
    detail::println(
      stream, "      {:24} print program version", *CommonArguments::version);
  }

  detail::println(
    stream,
    "\nFor more information, run:\n\n  {} COMMAND {}",
    get_prefix_for_user_messages<Traits>(argv),
    *CommonArguments::long_help);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const help_requested&,
  const program_info& info,
  argv_range auto&& argv,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  show_subcommand_usage<Traits, Ts...>(info, argv, outputStream);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const version_requested&,
  const program_info& info,
  argv_range auto&&,
  FILE* outputStream,
  [[maybe_unused]] FILE*) {
  detail::println(outputStream, "{}", info.mVersion);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const missing_required_argument&,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: You must specify a COMMAND",
    get_prefix_for_user_messages<Traits>(argv));
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const invalid_argument_value& r,
  const program_info&,
  argv_range auto&& argv,
  [[maybe_unused]] FILE* outputStream,
  FILE* errorStream) {
  detail::print(
    errorStream,
    "{}: `{}` is not a valid COMMAND",
    get_prefix_for_user_messages<Traits>(argv),
    r.mSource.mValue);
}

template <parsing_traits Traits, subcommand... Ts>
void print_incomplete_command_parse_reason(
  const incomplete_command_parse_reason_t& variant,
  const program_info& info,
  argv_range auto&& argv,
  FILE* outputStream,
  FILE* errorStream) {
  std::visit(
    [&]<class R>(R&& it) {
      print_incomplete_command_parse_reason<Traits, Ts...>(
        std::forward<R>(it), info, argv, outputStream, errorStream);
      if constexpr (std::decay_t<R>::is_error) {
        detail::print(errorStream, "\n\n");
        show_subcommand_usage<Traits, Ts...>(info, argv, errorStream);
      }
    },
    variant);
}

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <parsing_traits Traits, subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& info = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto ret = parse_subcommands_silent<Traits, First, Rest...>(argv, info);
  if (ret) [[likely]] {
    return ret;
  }

  // Matched a subcommand, but argument/option parsing failed for the subcommand

  // Skip over argv[1], as well as argv[0]
  // 2025-12-14: VS2022 requires that this be declared outside the lambda
  struct InnerTraits : Traits, detail::prefix_args_count_trait<2> {};

  std::visit(
    detail::overloaded {
      [&](const incomplete_command_parse_reason_t& reason) {
        detail::print_incomplete_command_parse_reason<Traits, First, Rest...>(
          reason, info, argv, outputStream, errorStream);
      },
      [&]<class T>(const incomplete_subcommand_parse_reason_t<T>& reason) {
        detail::print_incomplete_parse_reason<
          typename T::arguments_type,
          InnerTraits>(reason.value(), info, argv, outputStream, errorStream);
      },
    },
    ret.error());
  return ret;
}

template <subcommand First, subcommand... Rest>
auto parse_subcommands(
  detail::argv_range auto&& argv,
  const program_info& info = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<gnu_style_parsing_traits, First, Rest...>(
    argv, info, outputStream, errorStream);
}

template <class... Args>
auto parse_subcommands(
  const int argc,
  char** argv,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  return parse_subcommands<Args...>(
    std::views::counted(argv, argc), help, outputStream, errorStream);
}

template <subcommand First, subcommand... Rest>
bool is_error(
  const std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>& reason) {
  return std::visit(
    detail::overloaded {
      [](const incomplete_command_parse_reason_t& r) { return is_error(r); },
      []<class T>(const incomplete_subcommand_parse_reason_t<T>& r) {
        return is_error(*r);
      },
    },
    reason);
}

template <class T>
concept invocable_subcommand = subcommand<T>
  && requires(typename T::arguments_type&& args) { T::main(std::move(args)); };

template <class T, class TOther>
concept compatible_invocable_subcommand
  = invocable_subcommand<T> && invocable_subcommand<TOther>
  && std::same_as<
      std::invoke_result_t<decltype(T::main), typename T::arguments_type&&>,
      std::invoke_result_t<
        decltype(TOther::main),
        typename TOther::arguments_type&&>>;

template <
  parsing_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class TSuccess
  = std::invoke_result_t<typename First::main, typename First::argument_type&&>,
  class TIncomplete = std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected invoke_subcommands_silent(
  detail::argv_range auto&& argv,
  const program_info& info = {}) {
  auto result = parse_subcommands_silent<Traits, First, Rest...>(argv, info);
  if (!result) [[unlikely]] {
    return {std::unexpect, std::move(result).error()};
  }

  return {
    std::in_place,
    std::visit(
      []<class T>(subcommand_match<T>&& match) {
        return std::invoke(T::main, std::move(match).value());
      },
      std::move(result).value()),
  };
}

template <
  parsing_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class TSuccess = std::
    invoke_result_t<decltype(First::main), typename First::arguments_type&&>,
  class TIncomplete = std::variant<
    incomplete_command_parse_reason_t,
    incomplete_subcommand_parse_reason_t<First>,
    incomplete_subcommand_parse_reason_t<Rest>...>,
  class TExpected = std::expected<TSuccess, TIncomplete>>
TExpected invoke_subcommands(
  detail::argv_range auto&& argv,
  const program_info& info = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  auto result = parse_subcommands<Traits, First, Rest...>(
    argv, info, outputStream, errorStream);
  if (!result) [[unlikely]] {
    return std::unexpected {std::move(result).error()};
  }

  return std::visit(
    []<class T>(subcommand_match<T>&& match) {
      if constexpr (std::is_void_v<TSuccess>) {
        std::invoke(T::main, std::move(match).value());
        return TExpected {};
      } else {
        return std::invoke(T::main, std::move(match).value());
      }
    },
    std::move(result).value());
}

template <
  parsing_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class... Args>
auto invoke_subcommands_silent(const int argc, char** argv, Args&&... args) {
  return invoke_subcommands_silent<Traits, First, Rest...>(
    std::views::counted(argv, argc), std::forward<Args>(args)...);
}

template <
  parsing_traits Traits,
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class... Args>
auto invoke_subcommands(const int argc, char** argv, Args&&... args) {
  return invoke_subcommands<Traits, First, Rest...>(
    std::views::counted(argv, argc), std::forward<Args>(args)...);
}

template <
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class... Args>
auto invoke_subcommands_silent(Args&&... args) {
  return invoke_subcommands_silent<gnu_style_parsing_traits, First, Rest...>(
    std::forward<Args>(args)...);
}

template <
  invocable_subcommand First,
  compatible_invocable_subcommand<First>... Rest,
  class... Args>
auto invoke_subcommands(Args&&... args) {
  return invoke_subcommands<gnu_style_parsing_traits, First, Rest...>(
    std::forward<Args>(args)...);
}

}// namespace magic_args::inline public_api