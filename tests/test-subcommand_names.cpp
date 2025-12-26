// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#ifndef TEST_SINGLE_HEADER
#include <magic_args/subcommands.hpp>
#endif

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

struct Expected {
  std::string_view without_traits;
  std::string_view appended;
  std::string_view reflected;
};

struct UpperCamelCaseCommand {
  static constexpr Expected expected {
    .without_traits = "upper-camel-case",
    .appended = "UpperCamelCaseCommandAppend",
    .reflected = "RFUCC",
  };
};
struct snake_case_command {
  static constexpr Expected expected {
    .without_traits = "snake-case",
    .appended = "snake_case_commandAppend",
    .reflected = "RFSNC",
  };
};
struct ExplicitCommand {
  static constexpr auto name = "literal-value";
  static constexpr Expected expected {
    .without_traits = "literal-value",
    .appended = "literal-value",
    .reflected = "literal-value",
  };
};

namespace MyNS {
struct NamespacedCommand {
  static constexpr Expected expected {
    .without_traits = "namespaced",
    // Intentionally keep the namespace here; if someone's doing a custom
    // constexpr normalizer, they *might* want it, and if not, it's easy
    // enough to get rid of
    .appended = "MyNS::NamespacedCommandAppend",
    .reflected = "RFNSNSC",
  };
};
};// namespace MyNS

struct AppendTraits {
  template <class Command, auto Name>
  static constexpr auto normalize_subcommand_name() {
    using namespace magic_args::detail::constexpr_strings;
    constexpr auto withTrailingNull
      = concat<concat_byte_array_traits>(Name, "Append").get_buffer();
    std::array<char, withTrailingNull.size() - 1> ret {};
    std::ranges::copy_n(withTrailingNull.begin(), ret.size(), ret.begin());
    return ret;
  }
};

struct ReflectTraits {
  template <class Command, auto Name>
  static constexpr auto normalize_subcommand_name() {
    return Command::expected.reflected;
  }
};

using magic_args::detail::subcommand_name;

TEMPLATE_TEST_CASE(
  "without traits",
  "",
  UpperCamelCaseCommand,
  snake_case_command,
  ExplicitCommand,
  MyNS::NamespacedCommand) {
  struct EmptyTraits {};
  static constexpr auto buffer = subcommand_name<EmptyTraits, TestType>();
  CHECK(std::string_view {buffer} == TestType::expected.without_traits);
}

TEMPLATE_TEST_CASE(
  "append trait",
  "",
  UpperCamelCaseCommand,
  snake_case_command,
  ExplicitCommand,
  MyNS::NamespacedCommand) {
  static constexpr auto buffer = subcommand_name<AppendTraits, TestType>();
  CHECK(std::string_view {buffer} == TestType::expected.appended);
}

TEMPLATE_TEST_CASE(
  "reflect trait",
  "",
  UpperCamelCaseCommand,
  snake_case_command,
  ExplicitCommand,
  MyNS::NamespacedCommand) {
  static constexpr auto buffer = subcommand_name<ReflectTraits, TestType>();
  CHECK(std::string_view {buffer} == TestType::expected.reflected);
}
