// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <magic_args/magic_args.hpp>
#include <ranges>

#include "output.hpp"

struct OnlyFlags {
  bool mFoo {false};
  bool mBar {false};
  magic_args::flag mBaz {
    "baz",
    "do the bazzy thing",
    "b",
  };

  bool operator==(const OnlyFlags&) const noexcept = default;
};

constexpr char testName[] = "C:/Foo/Bar/my_test.exe";

struct EmptyStruct {};

TEST_CASE("empty struct, no args") {
  std::vector<std::string_view> argv {testName};
  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(argv, {}, out, err);
  CHECK(args.has_value());
  STATIC_CHECK(std::same_as<const EmptyStruct&, decltype(*args)>);
  CHECK(out.empty());
  CHECK(err.empty());
}

TEST_CASE("empty struct, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Options:

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("empty struct, --help with description") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(
    argv,
    {
      .mDescription = "Tests things.",
    },
    out,
    err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]
Tests things.

Options:

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("empty struct, --help with examples") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(
    argv,
    {
      .mExamples = {
        "my_test --foo",
        "my_test --bar",
      },
    },
    out,
    err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Examples:

  my_test --foo
  my_test --bar

Options:

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("empty struct, --help with description and examples") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(
    argv,
    {
      .mDescription = "Tests things.",
      .mExamples = {
        "my_test --foo",
        "my_test --bar",
      },
    },
    out,
    err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]
Tests things.

Examples:

  my_test --foo
  my_test --bar

Options:

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("empty struct, --help with version") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(
    argv,
    {
      .mVersion = "MyApp v1.2.3",
    },
    out,
    err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Options:

  -?, --help                   show this message
      --version                print program version
)EOF"[1]);
}

TEST_CASE("empty struct, --version") {
  std::vector<std::string_view> argv {testName, "--version"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(
    argv,
    {
      .mVersion = "MyApp v1.2.3",
    },
    out,
    err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::VersionRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == "MyApp v1.2.3\n");
}

TEST_CASE("flags only, no args") {
  std::vector<std::string_view> argv {testName};

  Output out, err;
  const auto args = magic_args::parse<OnlyFlags>(argv, {}, out, err);
  CHECK(args.has_value());
  CHECK(*args == OnlyFlags {});
  CHECK(out.empty());
  CHECK(err.empty());
}

TEST_CASE("flags only, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<OnlyFlags>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(args.error() == magic_args::incomplete_parse_reason::HelpRequested);

  REQUIRE(err.empty());
  REQUIRE(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --foo
      --bar
  -b, --baz                    do the bazzy thing

  -?, --help                   show this message
)EOF"[1]);
}
