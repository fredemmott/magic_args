// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "magic_args/powershell_style_parsing_traits.hpp"
#endif

#include "../cmake-build-debug-wsl/include/magic_args/Debug/single-header/magic_args/magic_args.hpp"
#include "arg-type-definitions.hpp"
#include "chomp.hpp"
#include "output.hpp"

template <magic_args::parsing_traits T>
struct BasicArgs {
  using parsing_traits = T;
  std::string mString;
  bool mFlag {false};
  magic_args::flag mDocumentedFlag {
    .mHelp = "This flag is documented",
    .mShortName = "d",
  };
};
using GNUArgs = BasicArgs<magic_args::gnu_style_parsing_traits>;
using PSArgs = BasicArgs<magic_args::powershell_style_parsing_traits>;

TEST_CASE("help, GNU-style") {
  std::vector<std::string_view> argv {"test_app", GENERATE("--help", "-?")};
  Output out, err;
  const auto args = magic_args::parse<GNUArgs>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: test_app [OPTIONS...]

Options:

      --string=VALUE
      --flag
  -d, --documented-flag        This flag is documented

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("help, powershell-style") {
  std::vector<std::string_view> argv {"test_app", "-Help"};
  Output out, err;
  const auto args = magic_args::parse<PSArgs>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: test_app [OPTIONS...]

Options:

      -String=VALUE
      -Flag
  -d, -DocumentedFlag          This flag is documented

  -?, -Help                    show this message
)EOF"));
}

// not testing GNU-style here as that's tested in test.cpp

TEST_CASE("args, powershell-style") {
  std::vector<std::string_view> argv {
    "test_app",
    "-String",
    "stringValue",
    "-Flag",
    "-DocumentedFlag",
  };
  Output out, err;
  const auto args = magic_args::parse<PSArgs>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mString == "stringValue");
  CHECK(args->mFlag);
  CHECK(args->mDocumentedFlag);
}
TEST_CASE("PowerShell-style invalid value") {
  constexpr std::string_view argv[] {
    "my_test", "-Raw", MyValueType::InvalidValue};

  Output out, err;
  const auto args = magic_args::parse<CustomArgsPS>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.get() == chomp(R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `-Raw` (seen: `-Raw ___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [POSITIONAL]

Options:

      -Raw=VALUE
      -Option=VALUE            std::optional

  -?, -Help                    show this message

Arguments:

      POSITIONAL
)EOF"));

  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "-Raw");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("PowerShell-style normalization") {
  std::vector<std::string_view> argv {"my_test", "-Help"};

  Output out, err;
  static_assert(magic_args::has_parsing_traits<NormalizationPS>);
  const auto args = magic_args::parse<NormalizationPS>(argv, out, err);
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      -EmUpperCamel=VALUE
      -EmUnderscoreUpperCamel=VALUE
      -UnderscoreUpperCamel=VALUE
      -UnderscoreLowerCamel=VALUE
      -UpperCamel=VALUE
      -LowerCamel=VALUE
      -EmSnakeCase=VALUE
      -SnakeCase=VALUE

  -?, -Help                    show this message
)EOF"));
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
}

TEST_CASE("GNU-style normalization") {
  std::vector<std::string_view> argv {"my_test", "--help"};

  Output out, err;
  const auto args = magic_args::parse<Normalization>(argv, out, err);
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --em-upper-camel=VALUE
      --em-underscore-upper-camel=VALUE
      --underscore-upper-camel=VALUE
      --underscore-lower-camel=VALUE
      --upper-camel=VALUE
      --lower-camel=VALUE
      --em-snake-case=VALUE
      --snake-case=VALUE

  -?, --help                   show this message
)EOF"));
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
}

TEST_CASE("GNU-style verbatim names") {
  std::vector<std::string_view> argv {"my_test", "--help"};

  Output out, err;
  const auto args = magic_args::parse<BasicNormalization<
    magic_args::verbatim_names<magic_args::gnu_style_parsing_traits>>>(
    argv, out, err);
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --mEmUpperCamel=VALUE
      --m_EmUnderscoreUpperCamel=VALUE
      --_UnderscoreUpperCamel=VALUE
      --_underscoreLowerCamel=VALUE
      --UpperCamel=VALUE
      --lowerCamel=VALUE
      --m_em_snake_case=VALUE
      --snake_case=VALUE

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("PowerShell-style verbatim names") {
  std::vector<std::string_view> argv {"my_test", "-Help"};

  Output out, err;
  const auto args = magic_args::parse<BasicNormalization<
    magic_args::verbatim_names<magic_args::powershell_style_parsing_traits>>>(
    argv, out, err);
  CHECK(err.get() == "");
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      -mEmUpperCamel=VALUE
      -m_EmUnderscoreUpperCamel=VALUE
      -_UnderscoreUpperCamel=VALUE
      -_underscoreLowerCamel=VALUE
      -UpperCamel=VALUE
      -lowerCamel=VALUE
      -m_em_snake_case=VALUE
      -snake_case=VALUE

  -?, -Help                    show this message
)EOF"));
}
