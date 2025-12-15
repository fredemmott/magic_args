// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include "chomp.hpp"

#include <catch2/matchers/catch_matchers_string.hpp>
#include "output.hpp"

namespace TestSubcommands {
struct CommandFooBar {
  static constexpr auto name = "foo";
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
    friend bool operator==(const arguments_type& a, const arguments_type& b) {
      return a.mBar == b.mBar && a.mBaz == b.mBaz;
    }
  };
};

struct CommandHerp {
  static constexpr auto name = "herp";
  static magic_args::program_info subcommand_info() noexcept {
    return {
      .mDescription = "Do the derpy thing",
      .mVersion = "Herp v1.2.3",
    };
  }
  struct arguments_type {
    std::string mDerp;
    friend bool operator==(const arguments_type& a, const arguments_type& b) {
      return a.mDerp == b.mDerp;
    }
  };
};
}// namespace TestSubcommands
using namespace TestSubcommands;

TEST_CASE("match foo subcommand: success") {
  constexpr std::array argv {"myApp", "foo", "--bar=BAR", "--baz=BAZ"};
  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
  CHECK(out.empty());
  CHECK(err.empty());
}

TEST_CASE("match herp subcommand: success") {
  constexpr std::array argv {"myApp", "herp", "--derp=DERP"};

  Output out, err;
  const auto ret = magic_args::parse_subcommands<CommandFooBar, CommandHerp>(
    argv, {}, out, err);
  const auto silent
    = magic_args::parse_subcommands_silent<CommandFooBar, CommandHerp>(
      argv, {});
  CHECK(ret == silent);
  CHECK(out.empty());
  CHECK(err.empty());
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
      herp                     Do the derpy thing

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
      herp                     Do the derpy thing

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
Do the derpy thing

Options:

      --derp=VALUE

  -?, --help                   show this message
      --version                print program version
)EOF"));
}