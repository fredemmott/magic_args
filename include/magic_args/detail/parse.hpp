// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_PARSE_HPP
#define MAGIC_ARGS_DETAIL_PARSE_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/argument_definitions.hpp>
#include <magic_args/incomplete_parse_reason.hpp>

#include "from_string.hpp"
#include "overloaded.hpp"
#include "visitors.hpp"
#endif

#include <algorithm>
#include <expected>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>

#ifndef __cpp_lib_expected
static_assert(
  false,
  "Your <expected> header is not compatible with your compiler; if you are "
  "using clang, use libc++");
#endif

namespace magic_args::detail {

// This exists because we can't declare `static constexpr` variables in structs
// defined inside functions
template <size_t N>
struct skip_args_count_trait {
  static constexpr std::size_t skip_args_count = N;
};

template <parsing_traits Traits>
constexpr std::size_t skip_args_count() {
  if constexpr (requires {
                  {
                    Traits::skip_args_count
                  } -> std::convertible_to<std::size_t>;
                }) {
    return Traits::skip_args_count;
  } else {
    // By default, skip argv[0]
    return 1;
  }
}

/** Usually just argv[0], but may be something else,
 *
 * e.g. the concatenation of both argv[0] and argv[1] if using subcommands.
 *
 * **DO NOT USE THIS TO LAUNCH SUBPROCESSES** - it does not escape the strings.
 */
template <parsing_traits Traits>
std::string get_prefix_for_user_messages(argv_range auto&& argv) {
  auto prefix
    = std::filesystem::path {*std::ranges::begin(argv)}.stem().string();
  for (std::size_t i = 1; i < detail::skip_args_count<Traits>(); ++i) {
    const auto it = std::ranges::begin(argv) + i;
    if (it >= std::ranges::end(argv)) {
      break;
    }
    prefix = std::format("{} {}", prefix, *it);
  }
  return prefix;
}

template <parsing_traits Traits, static_basic_argument TDef>
std::string provided_argument_name(const std::string_view arg) {
  if constexpr (is_positional_argument(TDef::behavior)) {
    return std::string {TDef::name};
  } else {
    const auto index = arg.find(Traits::value_separator);
    if (index == std::string_view::npos) {
      return std::string {arg};
    }
    return std::string {arg.substr(0, index)};
  }
}

template <parsing_traits Traits, static_basic_argument TDef>
auto map_value_parse_error(
  std::span<const std::string_view> args,
  const std::string_view value,
  invalid_argument_value e) {
  if (!e.source.empty()) {
    throw std::logic_error(
      "argument value parsers should not set error source");
  }
  e.source = {
    .argv_slice = std::ranges::to<std::vector<std::string>>(args),
    .name = provided_argument_name<Traits, TDef>(*std::ranges::begin(args)),
    .value = std::string {value},
  };
  return std::move(e);
}

struct consume_result {
  // The consumed portion
  std::string_view head;
  // The remainder
  std::string_view tail;
};

[[nodiscard]] inline consume_result consume(
  std::string_view& view,
  const std::size_t size) {
  if (size > view.size()) [[unlikely]] {
    throw std::out_of_range {std::format(
      "Asked to consume {} bytes, but only {} available", size, view.size())};
  }
  const auto head = view.substr(0, size);
  view.remove_prefix(size);
  return {head, view};
};

[[nodiscard]] inline std::optional<consume_result> consume(
  std::string_view& view,
  const std::string_view prefix) {
  if (prefix.empty()) {
    return std::nullopt;
  }
  if (!view.starts_with(prefix)) {
    return std::nullopt;
  }
  const auto head = view.substr(0, prefix.size());
  view.remove_prefix(prefix.size());
  return consume_result {head, view};
}

using argument_not_matched = std::monostate;
struct argument_match final {
  std::size_t consumed_argc {};
};

using option_match_result = std::variant<
  argument_not_matched,
  argument_match,
  missing_argument_value,
  invalid_argument_value>;

using parse_argument_result
  = std::expected<argument_match, incomplete_parse_reason_t>;

template <parsing_traits Traits, static_basic_option TDef>
struct match_long_option_t {
  static option_match_result operator()(
    const std::span<const std::string_view> args,
    typename TDef::value_type& out) {
    const auto arg = args[0];

    auto tail = arg;
    if (!consume(tail, Traits::long_arg_prefix)) {
      return argument_not_matched {};
    }

    const auto name = consume(tail, TDef::name);
    if (!name) {
      return argument_not_matched {};
    }

    if (tail.empty()) {
      if (std::ranges::size(args) == 1) {
        return missing_argument_value {
          .source = {
            .name = std::string { name->head },
            .argv_element = std::string { arg },
          },
        };
      }

      if (const auto ok = from_string(out, args[1]); !ok) [[unlikely]] {
        return map_value_parse_error<Traits, TDef>(args, args[1], ok.error());
      }
      return argument_match {.consumed_argc = 2};
    }

    if (!consume(tail, Traits::value_separator)) {
      // arg is `--foobar`, but we're trying to match `--foo`
      return argument_not_matched {};
    }

    // --foo=value
    if (const auto ok = from_string(out, tail); !ok) [[unlikely]] {
      return map_value_parse_error<Traits, TDef>(args, tail, ok.error());
    }

    return argument_match {.consumed_argc = 1};
  }
};

template <parsing_traits Traits, static_basic_option TDef>
[[nodiscard]]
option_match_result match_long_option(
  const std::span<const std::string_view> args,
  typename TDef::value_type& out) {
  return match_long_option_t<Traits, TDef> {}(args, out);
}

template <parsing_traits Traits, static_flag TDef>
struct match_long_option_t<Traits, TDef> {
  static option_match_result operator()(
    const std::span<const std::string_view> args,
    typename TDef::value_type& out) {
    const auto arg = args[0];

    auto tail = arg;
    if (!consume(tail, Traits::long_arg_prefix)) {
      return argument_not_matched {};
    }

    std::optional<std::string_view> providedValue;
    if (const auto match = consume({tail}, TDef::name)) {
      if (match->tail.empty()) {
        out = true;
        return argument_match {.consumed_argc = 1};
      }
      if (match->tail.starts_with(Traits::value_separator)) [[unlikely]] {
        providedValue = match->tail;
      }
    }

    if constexpr (static_negatable_flag<TDef>) {
      if (const auto match = consume(tail, TDef::negated_flag_name)) {
        if (match->tail.empty()) {
          out = false;
          return argument_match {.consumed_argc = 1};
        }
        if (match->tail.starts_with(Traits::value_separator)) [[unlikely]] {
          providedValue = match->tail;
        }
      }
    }

    if (providedValue) [[unlikely]] {
      return map_value_parse_error<Traits, TDef>(args, *providedValue, {});
    }

    return argument_not_matched {};
  }
};

template <parsing_traits Traits, static_counted_flag TDef>
struct match_long_option_t<Traits, TDef> {
  static option_match_result operator()(
    const std::span<const std::string_view> args,
    typename TDef::value_type& out) {
    const auto arg = args[0];

    auto tail = arg;
    if (!consume(tail, Traits::long_arg_prefix)) {
      return argument_not_matched {};
    }

    if (!consume(tail, TDef::name)) {
      return argument_not_matched {};
    }
    if (tail.empty()) {
      ++out;
      return argument_match {.consumed_argc = 1};
    }

    if (!tail.starts_with(Traits::value_separator)) [[likely]] {
      return argument_not_matched {};
    }

    const auto ok = from_string(out, tail.substr(1));
    if (!ok) [[unlikely]] {
      return map_value_parse_error<Traits, TDef>(args, tail, ok.error());
    }

    return argument_match {.consumed_argc = 1};
  }
};

template <static_short_flag TDef>
void apply_as_flag(typename TDef::value_type& out) {
  out = true;
}

template <static_short_counted_flag TDef>
void apply_as_flag(typename TDef::value_type& out) {
  ++out;
}

template <class T>
concept can_apply_as_flag
  = requires(typename T::value_type& out) { apply_as_flag<T>(out); };

template <parsing_traits_with_short_args Traits, static_basic_option TDef>
  requires(!static_short_basic_option<TDef>)
[[nodiscard]]
option_match_result match_short_option(
  [[maybe_unused]] const std::span<const std::string_view>& args,
  [[maybe_unused]] typename TDef::value_type& out) {
  return argument_not_matched {};
}

template <parsing_traits_with_short_args Traits, static_short_basic_option TDef>
[[nodiscard]]
option_match_result match_short_option(
  const std::span<const std::string_view>& args,
  typename TDef::value_type& out) {
  auto tail = args[0];

  if (!consume(tail, Traits::short_arg_prefix)) {
    return argument_not_matched {};
  }

  if (tail != TDef::short_name) {
    return argument_not_matched {};
  }

  if constexpr (can_apply_as_flag<TDef>) {
    apply_as_flag<TDef>(out);
    return argument_match {.consumed_argc = 1};
  } else {
    if (std::ranges::size(args) == 1) {
      return missing_argument_value {
        .source = {
        .name = std::string { TDef::short_name },
          .argv_element = std::string { args[0] },
        },
      };
    }
    if (const auto ok = from_string(out, args[1]); !ok) [[unlikely]] {
      return map_value_parse_error<Traits, TDef>(args, tail, ok.error());
    }
    return argument_match {.consumed_argc = 2};
  }
}

template <parsing_traits Traits, static_basic_positional_argument TArgDef>
parse_argument_result parse_positional_argument(
  const random_access_range_of<std::string_view> auto& args,
  typename TArgDef::value_type& out) {
  using value_type = typename TArgDef::value_type;

  if (std::ranges::empty(args)) {
    if constexpr (is_required(TArgDef::behavior)) {
      return std::unexpected {
        missing_required_argument {std::string {TArgDef::name}}};
    } else {
      return argument_match {.consumed_argc = 0};
    }
  }

  if constexpr (vector_like<value_type>) {
    out.reserve(args.size());
    // As of 2025-12-12, Apple Clang on Github Actions does not support
    // `std::views::enumerate`
    for (std::size_t i = 0; i < args.size(); ++i) {
      const auto& arg = args[i];
      typename value_type::value_type v {};
      if (const auto parsed = from_string(v, arg); !parsed) {
        return std::unexpected {map_value_parse_error<Traits, TArgDef>(
          std::views::single(*(std::ranges::begin(args) + i)),
          arg,
          parsed.error())};
      }
      out.emplace_back(std::move(v));
    }
    return argument_match {.consumed_argc = args.size()};
  } else {
    if (const auto parsed = from_string(out, args.front()); !parsed) {
      return std::unexpected {map_value_parse_error<Traits, TArgDef>(
        std::views::take(args, 1), args.front(), parsed.error())};
    }
    return argument_match {.consumed_argc = 1};
  }
}

auto& project_storage(auto& member) {
  using member_type = std::remove_cvref_t<decltype(member)>;
  if constexpr (basic_argument<member_type>) {
    return member.storage;
  } else {
    return member;
  }
}

template <parsing_traits Traits, auto TFnIt, class TArgs>
parse_argument_result parse_option_impl(
  TArgs& argsOut,
  std::span<const std::string_view> remainingArgv) {
  option_match_result match {};
  std::ignore = visit_options<Traits>(
    [&]<class TDef>(const TDef&, auto& memberOut) {
      auto& out = project_storage(memberOut);

      const auto result
        = TFnIt(std::type_identity<TDef> {}, remainingArgv, out);
      if (holds_alternative<argument_not_matched>(result)) {
        return false;
      }
      match = result;
      return true;
    },
    argsOut);
  if (const auto p = get_if<argument_match>(&match)) {
    return *p;
  }
  return std::visit(
    overloaded {
      [](const argument_match&) -> parse_argument_result {
        std::unreachable();
      },
      [=](const argument_not_matched&) -> parse_argument_result {
        return std::unexpected {unrecognized_option {
          .source = {std::string {remainingArgv.front()}},
        }};
      },
      [](const incomplete_parse_reason auto& r) -> parse_argument_result {
        return std::unexpected {r};
      },
    },
    match);
}

template <parsing_traits Traits, class TArgs>
parse_argument_result parse_long_option(
  TArgs& argsOut,
  std::span<const std::string_view> remainingArgv) {
  return parse_option_impl<
    Traits,
    []<static_basic_option TArgDef>(
      std::type_identity<TArgDef>, const auto args, auto& argOut) {
      return match_long_option<Traits, TArgDef>(args, argOut);
    }>(argsOut, remainingArgv);
}

template <parsing_traits Traits, class TArgs>
parse_argument_result parse_short_option(
  TArgs& argsOut,
  std::span<const std::string_view> remainingArgv) {
  const auto exactMatch = parse_option_impl<
    Traits,
    []<static_basic_option TArgDef>(
      std::type_identity<TArgDef>, const auto args, auto& argOut) {
      return match_short_option<Traits, TArgDef>(args, argOut);
    }>(argsOut, remainingArgv);
  if constexpr (!Traits::single_char_short_args) {
    return exactMatch;
  } else {
    if (exactMatch) {
      return exactMatch;
    }
    if (!holds_alternative<unrecognized_option>(exactMatch.error())) {
      return exactMatch;
    }

    auto arg = remainingArgv.front();
    const auto tail = consume(arg, Traits::short_arg_prefix)->tail;
    for (const char c: tail) {
      const bool matched = detail::visit_all_defined_arguments<Traits>(
        overloaded {
          [c]<can_apply_as_flag TArgDef>(const TArgDef, auto& memberOut) {
            if (
              TArgDef::short_name.size() != 1
              || TArgDef::short_name.front() != c) {
              return false;
            }

            auto& valueOut = project_storage(memberOut);
            apply_as_flag<TArgDef>(valueOut);
            return true;
          },
          [](const auto, auto&) { return false; }},
        argsOut);
      if (!matched) [[unlikely]] {
        return exactMatch;
      }
    }
    return argument_match {.consumed_argc = 1};
  }
}

}// namespace magic_args::detail

#endif
