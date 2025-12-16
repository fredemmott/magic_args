// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "subcommand-definitions.hpp"
using namespace TestSubcommands;

TEST_CASE("non-invoked cases") {
  const auto argv = GENERATE(values({
    std::vector {"myApp"},
    std::vector {"myApp", "NOT_A_VALID_COMMAND"},
    std::vector {"myApp", "--help"},
    std::vector {"myApp", "--version"},
    std::vector {"myApp", "foo", "--help"},
    std::vector {"myApp", "foo", "--bar"},// missing value
    std::vector {"myApp", "foo", "--derp"},// arg for wrong command
    std::vector {"myApp", "herp", "--bar"},// ditto
  }));

  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(argv);
  CHECK(
    ret
    == magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      static_cast<int>(argv.size()), argv.data()));

  REQUIRE_FALSE(ret.has_value());
  CHECK(
    ret.error()
    == magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(argv)
         .error());
}

TEST_CASE("invoke foo subcommand, no args") {
  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "foo"});
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandFooBar --bar= --baz=");
}

TEST_CASE("invoke foo subcommand, both args") {
  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "foo", "--bar=BAR", "--baz=BAZ"});
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandFooBar --bar=BAR --baz=BAZ");
}

TEST_CASE("invoke bar subcommand, no args") {
  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "herp"}, {});
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandHerp --derp=");
}

TEST_CASE("invoke bar subcommand, arg") {
  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "herp", "--derp=DERP"}, {});
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandHerp --derp=DERP");
}

TEST_CASE("void returns") {
  using Foo = CommandReturnsVoid<CommandFooBar>;
  using Herp = CommandReturnsVoid<CommandHerp>;
  Foo::invocation.reset();
  const auto ret = magic_args::invoke_subcommands_silent<Foo, Herp>(
    std::array {"myApp", "foo", "--bar=TestBar"});
  CHECK(ret.has_value());
  STATIC_CHECK(std::is_void_v<std::decay_t<decltype(ret)>::value_type>);

  CHECKED_IF(Foo::invocation.has_value()) {
    CHECK(Foo::invocation->mBar == "TestBar");
  }
}

TEST_CASE("powershell-style success") {
  const magic_args::program_info info {
    .mVersion = "TestApp v1.2.3",
  };
  struct params_t {
    std::vector<const char*> gnuStyle;
    std::vector<const char*> powershellStyle;
  };
  const auto [gnuArgv, psArgv] = GENERATE(
    values<params_t>({
      {
        {"mytest", "--help"},
        {"mytest", "-Help"},
      },
      {
        {"mytest", "--version"},
        {"mytest", "-Version"},
      },
      {
        {"mytest", "foo", "--bar=TEST_BAR"},
        {"mytest", "foo", "-Bar", "TEST_BAR"},
      },
    }));

  const auto gnu
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      gnuArgv, info);
  const auto ps = magic_args::invoke_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv, info);
  CHECK(ps == gnu);
  const auto psWithArgc = magic_args::invoke_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(static_cast<int>(psArgv.size()), psArgv.data(), info);
  CHECK(psWithArgc == ps);
}

TEST_CASE("powershell-style non-invoked cases") {
  const auto argv = GENERATE(values({
    std::vector {"myApp"},
    std::vector {"myApp", "NOT_A_VALID_COMMAND"},
    std::vector {"myApp", "-Help"},
    std::vector {"myApp", "-Version"},
    std::vector {"myApp", "foo", "-Help"},
    std::vector {"myApp", "foo", "-Bar"},// missing value
    std::vector {"myApp", "foo", "-Derp"},// arg for wrong command
    std::vector {"myApp", "herp", "-Bar"},// ditto
  }));

  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(argv);
  CHECK(
    ret
    == magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      static_cast<int>(argv.size()), argv.data()));

  REQUIRE_FALSE(ret.has_value());
  CHECK(
    ret.error()
    == magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(argv)
         .error());
}
