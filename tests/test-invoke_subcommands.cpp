// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/generators/catch_generators.hpp>
#include "chomp.hpp"
#include "output.hpp"
#include "subcommand-definitions.hpp"

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
  Output out, err;
  using Foo = CommandReturnsVoid<CommandFooBar>;
  using Herp = CommandReturnsVoid<CommandHerp>;
  Foo::invocation.reset();
  const auto ret = magic_args::invoke_subcommands<Foo, Herp>(
    std::array {"myApp", "foo", "--bar=TestBar"}, out, err);
  CHECK(ret.has_value());
  CHECK(out.empty());
  CHECK(err.empty());
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
  Output out, err;
  const auto ret = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(
    argv, out, err);
  REQUIRE_FALSE(ret.has_value());

  Output parseOut, parseErr;
  const auto parseRet
    = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
      argv, parseOut, parseErr);
  REQUIRE_FALSE(parseRet.has_value());
  CHECK(ret.error() == parseRet.error());
  CHECK(out.get() == parseOut.get());
  CHECK(err.get() == parseErr.get());
}

TEST_CASE("powershell-style success (no output)") {
  constexpr std::array gnuArgv {"mytest", "foo", "--bar=TEST_BAR"};
  constexpr std::array psArgv {"mytest", "foo", "-Bar", "TEST_BAR"};
  Output out, err;

  const auto ps = magic_args::invoke_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());

  const auto gnu = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(
    gnuArgv, out, err);
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

  Output invokedOut, invokedErr;
  const auto invoked = magic_args::invoke_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, invokedOut, invokedErr);
  REQUIRE_FALSE(invoked.has_value());
  Output parsedOut, parsedErr;
  const auto parsed = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, parsedOut, parsedErr);
  REQUIRE_FALSE(parsed.has_value());

  CHECK(invoked.error() == parsed.error());
  CHECK(invokedOut.get() == parsedOut.get());
  CHECK(invokedErr.get() == parsedErr.get());
}

template <magic_args::parsing_traits T>
struct WithVersion {
  using parsing_traits = T;
  static constexpr auto version = "MyTest v1.2.3";
};

TEST_CASE("root version") {
  constexpr auto gnuArgv = std::array {"my_args", "--version"};
  constexpr auto psArgv = std::array {"my_args", "-Version"};

  Output gnuOut, gnuErr;
  const auto gnu = magic_args::invoke_subcommands<
    WithVersion<magic_args::gnu_style_parsing_traits>,
    CommandFooBar,
    CommandHerp>(gnuArgv, gnuOut, gnuErr);
  CHECK(
    gnu
    == magic_args::invoke_subcommands_silent<
      WithVersion<magic_args::gnu_style_parsing_traits>,
      CommandFooBar,
      CommandHerp>(gnuArgv));
  CHECK(gnuErr.empty());
  CHECK(gnuOut.get() == "MyTest v1.2.3\n");

  Output psOut, psErr;
  const auto ps = magic_args::invoke_subcommands<
    WithVersion<magic_args::powershell_style_parsing_traits>,
    CommandFooBar,
    CommandHerp>(psArgv, psOut, psErr);
  CHECK(
    ps
    == magic_args::invoke_subcommands_silent<
      WithVersion<magic_args::powershell_style_parsing_traits>,
      CommandFooBar,
      CommandHerp>(psArgv));
  CHECK(psOut.get() == gnuOut.get());
  CHECK(psErr.get() == gnuErr.get());
}