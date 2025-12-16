// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include "subcommand-definitions.hpp"
using namespace TestSubcommands;

template <class... Args>
void CheckErrorMatches(Args&&... args) {
  const auto ret
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      args...);
  REQUIRE_FALSE(ret.has_value());
  CHECK(
    ret.error()
    == magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(args...)
         .error());
};

TEST_CASE("non-invoked cases") {
  CheckErrorMatches(std::array {"myApp"});
  CheckErrorMatches(std::array {"myApp", "NOT_A_VALID_COMMAND"});
  CheckErrorMatches(std::array {"myApp", "--help"});
  CheckErrorMatches(std::array {"myApp", "--version"});
  CheckErrorMatches(std::array {"myApp", "foo", "--help"});
  CheckErrorMatches(std::array {"myApp", "foo", "--bar"});// missing value
  CheckErrorMatches(
    std::array {"myApp", "foo", "--derp"});// arg for wrong command
  CheckErrorMatches(std::array {"myApp", "herp", "--bar"});// ditto
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