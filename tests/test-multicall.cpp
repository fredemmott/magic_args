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

TEST_CASE("invoke foo") {
  const auto argv = GENERATE(values({
    std::vector {"foo", "--bar=BAR", "--baz=BAZ"},
    std::vector {"foo.exe", "--bar=BAR", "--baz=BAZ"},
    std::vector {".foo", "--bar=BAR", "--baz=BAZ"},
  }));

  const auto ret = magic_args::invoke_subcommands_silent<
    magic_args::multicall_traits<>,
    CommandFooBar,
    CommandHerp>(argv);
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandFooBar --bar=BAR --baz=BAZ");
}

TEST_CASE("invoke herp") {
  const auto argv = GENERATE(values({
    std::vector {"herp", "--derp=DERP"},
    std::vector {"herp.exe", "--derp=DERP"},
    std::vector {".herp", "--derp=DERP"},
  }));

  const auto ret = magic_args::invoke_subcommands_silent<
    magic_args::multicall_traits<>,
    CommandFooBar,
    CommandHerp>(argv);
  REQUIRE(ret.has_value());
  CHECK(*ret == "TEST RESULT CommandHerp --derp=DERP");
}

TEST_CASE("invalid command") {
  constexpr std::array argv {"mytest"};

  Output out, err;
  const auto ret = magic_args::invoke_subcommands<
    magic_args::multicall_traits<>,
    CommandFooBar,
    CommandHerp>(argv, {}, out, err);
  CHECK_FALSE(ret.has_value());
  CHECK(out.empty());
  CHECK(err.get() == chomp(R"EOF(
mytest: `mytest` is not a valid COMMAND

Usage: COMMAND [OPTIONS...]

Commands:

      foo
      herp                     Description goes here

  -?, --help                   show this message

For more information, run:

  COMMAND --help
)EOF"));
}

TEST_CASE("foo --help") {
  constexpr std::array argv {"foo", "--help"};

  Output out, err;
  const auto ret = magic_args::invoke_subcommands<
    magic_args::multicall_traits<>,
    CommandFooBar,
    CommandHerp>(argv, {}, out, err);
  CHECK_FALSE(ret.has_value());
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: foo [OPTIONS...]

Options:

      --bar=VALUE
      --baz=VALUE

  -?, --help                   show this message
)EOF"));
}
