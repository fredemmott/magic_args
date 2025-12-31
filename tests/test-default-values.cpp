// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>
#include "chomp.hpp"
#include "test_output.hpp"

struct MyCustomType {
  std::string mValue;
};
std::expected<void, magic_args::invalid_argument_value> from_argument_value(
  MyCustomType& value,
  const std::string_view arg) {
  value.mValue = std::string {arg};
  return {};
}
auto to_argument_value(const MyCustomType& value) {
  return value.mValue;
}

struct WithDefaults {
  std::string mMyArg {"testValue"};
  magic_args::option<std::string> mMyArgWithHelp {
    .storage = "testValue2",
    .help = "Test help text",
  };
  MyCustomType mMyCustomType {"testValue3"};
};

TEST_CASE("default argument value - no options") {
  const auto noOptions
    = magic_args::parse_silent<WithDefaults>(std::array {"mytest"});
  REQUIRE(noOptions.has_value());
  CHECK(noOptions->mMyArg == "testValue");
  CHECK(noOptions->mMyArgWithHelp == "testValue2");
  CHECK(noOptions->mMyCustomType.mValue == "testValue3");
}

TEST_CASE("default argument value - overriden") {
  const auto noOptions = magic_args::parse_silent<WithDefaults>(
    std::array {"mytest", "--my-arg", "foobar"});
  REQUIRE(noOptions.has_value());
  CHECK(noOptions->mMyArg == "foobar");
}

TEST_CASE("default argument value - --help") {
  test_output output;
  const auto result
    = magic_args::parse<WithDefaults>(std::vector {"mytest", "--help"}, output);
  CHECK(output.error_str().empty());
  CHECK(output.out_str() == chomp(R"EOF(
Usage: mytest [OPTIONS...]

Options:

      --my-arg=VALUE           (default: testValue)
      --my-arg-with-help=VALUE Test help text
                               (default: testValue2)
      --my-custom-type=VALUE   (default: testValue3)

  -?, --help                   show this message
)EOF"));

  REQUIRE_FALSE(result.has_value());
  REQUIRE(holds_alternative<magic_args::help_requested>(result.error()));
}
