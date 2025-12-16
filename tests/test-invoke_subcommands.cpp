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
    std::array {"myApp", "foo", "--bar=TestBar"}, {}, out, err);
  CHECK(ret.has_value());
  CHECK(out.empty());
  CHECK(err.empty());
  STATIC_CHECK(std::is_void_v<std::decay_t<decltype(ret)>::value_type>);

  CHECKED_IF(Foo::invocation.has_value()) {
    CHECK(Foo::invocation->mBar == "TestBar");
  }
}

TEST_CASE("output cases") {
  struct params_t {
    std::vector<std::string_view> argv;
    magic_args::program_info info {};
  };
  const auto [argv, info] = GENERATE(
    values<params_t>({
      {{"MyApp", "--help"}},
      {{"MyApp", "--version"}},
      {
        {"MyApp", "--version"},
        {.mDescription = "Test", .mVersion = "MyApp v1.2.3"},
      },
      {{"MyApp", "foo", "--help"}},
      {{"MyApp", "foo", "--bar"}},// missing argument value
      {{"MyApp", "foo", "--invalid"}},
    }));
  Output out, err;
  const auto ret = magic_args::invoke_subcommands<CommandFooBar, CommandHerp>(
    argv, info, out, err);
  REQUIRE_FALSE(ret.has_value());

  Output parseOut, parseErr;
  const auto parseRet
    = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
      argv, info, parseOut, parseErr);
  REQUIRE_FALSE(parseRet.has_value());
  CHECK(ret.error() == parseRet.error());
  CHECK(out.get() == parseOut.get());
  CHECK(err.get() == parseErr.get());
}