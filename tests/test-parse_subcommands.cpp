// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "chomp.hpp"
#include "output.hpp"
#include "subcommand-definitions.hpp"

using namespace TestSubcommands;

TEST_CASE("success (no output)") {
  const auto argv = GENERATE(
    values<std::vector<const char*>>({
      {"myApp", "foo"},
      {"myApp", "foo", "--bar=BAR", "--baz=BAZ"},
      {"myApp", "herp"},
      {"myApp", "herp", "--derp=DERP"},
    }));
  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());

  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
}

TEST_CASE("'--help' without subcommand") {
  constexpr std::array argv {"myApp", "--help"};

  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
  CHECK(holds_alternative<magic_args::help_requested>(ret.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: myApp COMMAND [OPTIONS...]

Commands:

      foo
      herp                     Description goes here

  -?, --help                   show this message

For more information, run:

  myApp COMMAND --help
)EOF"));
}

TEST_CASE("'--help' without subcommand, but with extra info") {
  constexpr std::array argv {"myApp", "--help"};

  const magic_args::program_info info {
    .mDescription = "Do stuff with subcommands",
    .mVersion = "MyApp v1.2.3",
  };

  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, info, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, info);
  CHECK(ret == silent);
  CHECK(holds_alternative<magic_args::help_requested>(ret.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: myApp COMMAND [OPTIONS...]
Do stuff with subcommands

Commands:

      foo
      herp                     Description goes here

  -?, --help                   show this message
      --version                print program version

For more information, run:

  myApp COMMAND --help
)EOF"));
}

TEST_CASE("'--version' as subcommand, when not defined") {
  constexpr std::array argv {"myApp", "--version"};
  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
  CHECK(out.empty());
  CHECK_THAT(err.get(), Catch::Matchers::StartsWith(std::string {chomp(R"EOF(
myApp: `--version` is not a valid COMMAND

Usage: myApp COMMAND [OPTIONS...]
)EOF")}));
}

TEST_CASE("'--version' as subcommand, when defined") {
  constexpr std::array argv {"myApp", "--version"};
  const magic_args::program_info info {
    .mVersion = "MyApp v1.2.3",
  };
  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, info, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, info);
  CHECK(ret == silent);
  CHECK(err.empty());
  CHECK(out.get() == "MyApp v1.2.3\n");
}

TEST_CASE("Subcommand --help") {
  constexpr std::array argv {"myApp", "herp", "--help"};
  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: myApp herp [OPTIONS...]
Description goes here

Options:

      --derp=VALUE

  -?, --help                   show this message
      --version                print program version
)EOF"));
}

TEST_CASE("powershell-style success (no output)") {
  constexpr std::array gnuArgv {"mytest", "foo", "--bar=TEST_BAR"};
  constexpr std::array psArgv {"mytest", "foo", "-Bar", "TEST_BAR"};
  Output out, err;

  const auto ps = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());

  const auto psSilent = magic_args::parse_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(psArgv, {});
  CHECK(ps == psSilent);

  const auto gnu = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    gnuArgv, {}, out, err);
  CHECK(ps == gnu);
}

TEST_CASE("powershell-style root help") {
  constexpr std::array argv {"mytest", "-Help"};
  const magic_args::program_info info {
    .mVersion = "MyApp v1.2.3",
  };
  Output out, err;
  const auto ret = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, info, out, err);
  const auto silent = magic_args::parse_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, info);
  CHECK(ret == silent);

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: mytest COMMAND [OPTIONS...]

Commands:

      foo
      herp                     Description goes here

  -?, -Help                    show this message
      -Version                 print program version

For more information, run:

  mytest COMMAND -Help
)EOF"));
}

TEST_CASE("powershell-style with missing subcommand") {
  constexpr std::array argv {"mytest"};
  Output out, err;
  const auto ret = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, {}, out, err);
  const auto silent = magic_args::parse_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv);
  CHECK(ret == silent);

  CHECK(out.empty());
  CHECK_THAT(
    err.get(), Catch::Matchers::ContainsSubstring("mytest COMMAND -Help"));
  CHECK_THAT(err.get(), !Catch::Matchers::ContainsSubstring("--help"));
}

TEST_CASE("powershell-style subcommand help") {
  constexpr std::array argv {"mytest", "foo", "-Help"};
  Output out, err;
  const auto ret = magic_args::parse_subcommands<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv, {}, out, err);
  const auto silent = magic_args::parse_subcommands_silent<
    magic_args::powershell_style_parsing_traits,
    CommandFooBar,
    CommandHerp>(argv);
  CHECK(ret == silent);

  CHECK(err.empty());
  CHECK_THAT(out.get(), Catch::Matchers::ContainsSubstring(" -Bar="));
  CHECK_THAT(out.get(), !Catch::Matchers::ContainsSubstring(" --bar"));
}