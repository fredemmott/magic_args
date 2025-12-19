// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>

#include "arg-type-definitions.hpp"
#include "chomp.hpp"
#include "output.hpp"

constexpr char testName[] = "C:/Foo/Bar/my_test.exe";

TEST_CASE("empty struct, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

  -?, --help                   show this message
)EOF"));
}

struct EmptyWithDescription {
  static constexpr auto description = "Tests things.";
};

TEST_CASE("empty struct, --help with description") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyWithDescription>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]
Tests things.

Options:

  -?, --help                   show this message
)EOF"));
}

struct EmptyWithExamples {
  static constexpr auto examples = {
    "my_test --foo",
    "my_test --bar",
  };
};

TEST_CASE("empty struct, --help with examples") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyWithExamples>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Examples:

  my_test --foo
  my_test --bar

Options:

  -?, --help                   show this message
)EOF"));
}

struct EmptyWithDescriptionAndExamples {
  static constexpr auto description = "Tests things.";
  static constexpr auto examples = {
    "my_test --foo",
    "my_test --bar",
  };
};

TEST_CASE("empty struct, --help with description and examples") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<EmptyWithDescriptionAndExamples>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]
Tests things.

Examples:

  my_test --foo
  my_test --bar

Options:

  -?, --help                   show this message
)EOF"));
}

struct EmptyWithVersion {
  static constexpr auto version = "MyApp v1.2.3";
};

TEST_CASE("empty struct, --help with version") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyWithVersion>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

  -?, --help                   show this message
      --version                print program version
)EOF"));
}

TEST_CASE("flags only, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<FlagsOnly>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --foo
      --bar
  -b, --baz                    do the bazzy thing

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("options only, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<OptionsOnly>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --string=VALUE
      --int=VALUE
  -f, --foo=VALUE              do the foo thing

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("parameters, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...] [--] [INPUT] [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"));
}

TEST_CASE("mandatory named parameter, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryPositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...] [--] INPUT [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"));
}

TEST_CASE("multi-value parameter - --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<MultiValuePositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == chomp(R"EOF(
Usage: my_test [OPTIONS...] [--] [OUTPUT] [INPUT [INPUT [...]]]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      OUTPUT                   file to create
      INPUTS
)EOF"));
}
