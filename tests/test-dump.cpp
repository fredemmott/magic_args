// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>

#include "chomp.hpp"
#include "test_output.hpp"

enum class MyEnum {
  Foo,
  Bar,
  Baz,
};

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

struct Args {
  std::string mString;
  std::optional<int> mOptionalInt;
  MyEnum mEnum;
  magic_args::option<std::string> mOption {
    .mShortName = "o",
  };
  magic_args::flag mFlag;
  magic_args::counted_flag mVerbose {
    .mShortName = "v",
  };
  MyCustomType mCustomType;
  magic_args::optional_positional_argument<std::string> mPositional;
};

TEST_CASE("defaults") {
  test_output output;
  const auto args = magic_args::parse<Args>(std::array {"mytest"}, output);
  CHECK(output.empty());
  REQUIRE(args.has_value());
  magic_args::dump(*args, (output).out);
  CHECK(output.out_str() == chomp(R"EOF(
mString                       ``
mOptionalInt                  `[nullopt]`
mEnum                         `Foo`
mOption                       ``
mFlag                         `false`
mVerbose                      `0`
mCustomType                   ``
mPositional                   ``
)EOF"));
}

TEST_CASE("all") {
  const auto args = magic_args::parse_silent<Args>(std::array {
    "mytest",
    "--string=TestString",
    "--optional-int=42",
    "--enum=Bar",
    "--option=TestOption",
    "--flag",
    "-vvv",
    "--custom-type=TestCustomValue",
    "Derp",
  });
  REQUIRE(args.has_value());
  test_output output;
  magic_args::dump(*args, (output).out);
  CHECK(output.out_str() == chomp(R"EOF(
mString                       `TestString`
mOptionalInt                  `42`
mEnum                         `Bar`
mOption                       `TestOption`
mFlag                         `true`
mVerbose                      `3`
mCustomType                   `TestCustomValue`
mPositional                   `Derp`
)EOF"));
}
