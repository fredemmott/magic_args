// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/generators/catch_generators.hpp>
#include "chomp.hpp"
#include "subcommand-definitions.hpp"
#include "test_output.hpp"

using namespace TestSubcommands;

TEST_CASE("silent cases") {
  const auto argv = GENERATE(values({
    std::vector {"MyApp", "foo"},
    std::vector {"myApp", "foo", "--bar=BAR", "--baz=BAZ"},
    std::vector {"myApp", "herp"},
    std::vector {"myApp", "herp", "--derp=DERP"},
    std::vector {"myApp", "herp", "--derp", "DERP"},
  }));

  const auto ret
    = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(argv);
  REQUIRE(ret.has_value());
  CHECK(
    ret.value()
    == magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(argv));
}

TEST_CASE("void returns") {
  using Foo = CommandReturnsVoid<CommandFooBar>;
  using Herp = CommandReturnsVoid<CommandHerp>;
  Foo::invocation.reset();
  const auto ret = magic_args::invoke_subcommands_silent<Foo, Herp>(
    std::array {"myApp", "foo", "--bar=TestBar"});
  REQUIRE(ret.has_value());
  STATIC_CHECK(std::is_void_v<std::decay_t<decltype(ret)>::value_type>);

  CHECK(Foo::invocation.has_value());
  if (Foo::invocation.has_value()) {
    CHECK(Foo::invocation->mBar == "TestBar");
  }
}

TEST_CASE("output cases") {
  const auto argv = GENERATE(
    values<std::vector<const char*>>({
      {"MyApp", "--help"},
      {"MyApp", "foo", "--help"},
      {"MyApp", "foo", "--bar"},// missing argument value
      {"MyApp", "foo", "--invalid"},
      {"MyApp", "foo", "--version"},
    }));
  test_output output;
  const auto ret
    = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(argv, output);
  REQUIRE_FALSE(ret.has_value());

  test_output parseOutput;
  const auto parseRet
    = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
      argv, parseOutput);
  REQUIRE_FALSE(parseRet.has_value());
  CHECK(ret.error() == parseRet.error());
  CHECK(output.out_str() == parseOutput.out_str());
  CHECK(output.error_str() == parseOutput.error_str());
}

TEST_CASE("powershell-style success (no output)") {
  constexpr std::array gnuArgv {"mytest", "foo", "--bar=TEST_BAR"};
  constexpr std::array psArgv {"mytest", "foo", "-Bar", "TEST_BAR"};

  const auto ps = magic_args::invoke_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv);

  const auto gnu
    = magic_args::invoke_subcommands_silent<CommandFooBar, CommandHerp>(
      gnuArgv);
  CHECK(ps == gnu);
}

TEST_CASE("powershell-style non-invoked") {
  const auto argv = GENERATE(
    values<std::vector<const char*>>({
      {"mytest"},
      {"mytest", "-Help"},
      {"mytest", "invalid"},
      {"mytest", "foo", "-Invalid"},
      {"mytest", "foo", "-Bar" /* missing value */},
      {"mytest", "herp", "-Invalid" /* missing value */},
    }));

  test_output invoked;
  const auto invokedRet = magic_args::invoke_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, invoked);
  REQUIRE_FALSE(invokedRet.has_value());
  test_output parsed;
  const auto parsedRet = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, parsed);
  REQUIRE_FALSE(parsedRet.has_value());

  CHECK(invokedRet.error() == parsedRet.error());
  CHECK(invoked.out_str() == parsed.out_str());
  CHECK(invoked.error_str() == parsed.error_str());
}

template <magic_args::parsing_traits T>
struct WithVersion {
  using parsing_traits = T;
  static constexpr auto version = "MyTest v1.2.3";
};

TEST_CASE("root version") {
  constexpr auto gnuArgv = std::array {"my_args", "--version"};
  constexpr auto psArgv = std::array {"my_args", "-Version"};

  test_output gnu;
  const auto gnuRet = magic_args::invoke_subcommands<
    WithVersion<magic_args::gnu_style_parsing_traits>,
    CommandFooBar,
    CommandHerp>(gnuArgv, gnu);
  CHECK(
    gnuRet
    == magic_args::invoke_subcommands_silent<
      WithVersion<magic_args::gnu_style_parsing_traits>,
      CommandFooBar,
      CommandHerp>(gnuArgv));
  CHECK(gnu.error_str().empty());
  CHECK(gnu.out_str() == "MyTest v1.2.3\n");

  test_output ps;
  const auto psRet = magic_args::invoke_subcommands<
    WithVersion<magic_args::powershell_style_parsing_traits>,
    CommandFooBar,
    CommandHerp>(psArgv, ps);
  CHECK(
    psRet
    == magic_args::invoke_subcommands_silent<
      WithVersion<magic_args::powershell_style_parsing_traits>,
      CommandFooBar,
      CommandHerp>(psArgv));
  CHECK(ps.out_str() == gnu.out_str());
  CHECK(ps.error_str() == gnu.error_str());
}