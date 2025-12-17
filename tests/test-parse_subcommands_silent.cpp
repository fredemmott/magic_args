// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#ifdef TEST_SINGLE_HEADER
#define MAGIC_ARGS_ENABLE_SUBCOMMANDS
#include <magic_args/magic_args.hpp>
#else
#include <magic_args/magic_args.hpp>
#include <magic_args/subcommands.hpp>
#endif

#include <catch2/catch_test_macros.hpp>

#include <catch2/generators/catch_generators.hpp>
#include "subcommand-definitions.hpp"

using namespace TestSubcommands;

TEST_CASE("missing COMMAND -> missing_required_argument") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp"}, {});
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(
    holds_alternative<magic_args::missing_required_argument>(ret.error()));
}

TEST_CASE("'--help' as COMMAND -> help_requested") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "--help"}, {});
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(holds_alternative<magic_args::help_requested>(ret.error()));
}

TEST_CASE("'--version' as COMMAND -> version_requested when version provided") {
  const magic_args::program_info info {
    .mDescription = "Subcommands app",
    .mVersion = "1.2.3",
  };
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "--version"}, info);
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(holds_alternative<magic_args::version_requested>(ret.error()));
}

TEST_CASE("invalid COMMAND -> invalid_argument_value error on COMMAND") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "unknown"}, {});
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(ret.error()));
  const auto& e = get<magic_args::invalid_argument_value>(ret.error());
  CHECK(e.mSource.mName == std::string {"COMMAND"});
  CHECK(e.mSource.mValue == std::string {"unknown"});
}

TEST_CASE(
  "'--version' as COMMAND -> invalid_argument_value when version omitted") {
  const magic_args::program_info info {
    .mDescription = "Subcommands app",
  };
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "--version"}, info);
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(ret.error()));
}

TEST_CASE("match foo subcommand and parse its arguments (silent)") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "foo", "--bar=BAR", "--baz=BAZ"}, {});
  REQUIRE(ret.has_value());
  const auto& v = ret.value();
  REQUIRE(
    std::holds_alternative<magic_args::subcommand_match<CommandFooBar>>(v));
  const auto& match = std::get<magic_args::subcommand_match<CommandFooBar>>(v);
  CHECK(match->mBar == "BAR");
  CHECK(match->mBaz == "BAZ");
}

TEST_CASE("match herp subcommand and parse its arguments (silent)") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "herp", "--derp=DERP"}, {});
  REQUIRE(ret.has_value());
  const auto& v = ret.value();
  REQUIRE(std::holds_alternative<magic_args::subcommand_match<CommandHerp>>(v));
  const auto& match = std::get<magic_args::subcommand_match<CommandHerp>>(v);
  CHECK(match->mDerp == "DERP");
}

TEST_CASE("match first, but pass invalid arguments (silent)") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "foo", "--INVALID"});
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(
    holds_alternative<
      magic_args::incomplete_subcommand_parse_reason_t<CommandFooBar>>(
      ret.error()));
  const auto& tagged
    = get<magic_args::incomplete_subcommand_parse_reason_t<CommandFooBar>>(
      ret.error());
  REQUIRE(holds_alternative<magic_args::invalid_argument>(tagged.value()));
  const auto e = get<magic_args::invalid_argument>(tagged.value());
  CHECK(e.mKind == magic_args::invalid_argument::kind::Option);
  CHECK(e.mSource.mArg == "--INVALID");
}

TEST_CASE("match second, but pass invalid arguments (silent)") {
  const auto ret
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      std::array {"myApp", "herp", "--INVALID"});
  REQUIRE_FALSE(ret.has_value());
  REQUIRE(
    holds_alternative<
      magic_args::incomplete_subcommand_parse_reason_t<CommandHerp>>(
      ret.error()));
  const auto& tagged
    = get<magic_args::incomplete_subcommand_parse_reason_t<CommandHerp>>(
      ret.error());
  REQUIRE(holds_alternative<magic_args::invalid_argument>(tagged.value()));
  const auto e = get<magic_args::invalid_argument>(tagged.value());
  CHECK(e.mKind == magic_args::invalid_argument::kind::Option);
  CHECK(e.mSource.mArg == "--INVALID");
}

TEST_CASE("powershell-style") {
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
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      gnuArgv, info);
  const auto ps = magic_args::parse_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv, info);
  CHECK(ps == gnu);
}