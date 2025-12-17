// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>

#include "chomp.hpp"
#include "output.hpp"

enum class MyEnum {
  Foo,
  Bar,
  Baz,
};

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
  magic_args::optional_positional_argument<std::string> mPositional;
};

TEST_CASE("defaults") {
  Output out, err;
  const auto args
    = magic_args::parse<Args>(std::array {"mytest"}, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  magic_args::dump(*args, out);
  CHECK(out.get() == chomp(R"EOF(
mString                       ``
mOptionalInt                  `[nullopt]`
mEnum                         `Foo`
mOption                       ``
mFlag                         `false`
mVerbose                      `0`
mPositional                   ``
)EOF"));
}

TEST_CASE("all") {
  Output out, err;
  const auto args = magic_args::parse<Args>(
    std::array {
      "mytest",
      "--string=TestString",
      "--optional-int=42",
      "--enum=Bar",
      "--option=TestOption",
      "--flag",
      "-vvv",
      "Derp",
    },
    {},
    out,
    err);
  CHECK(out.empty());
  // CHECK(err.empty());
  CHECK(err.get() == "");
  REQUIRE(args.has_value());
  magic_args::dump(*args, out);
  CHECK(out.get() == chomp(R"EOF(
mString                       `TestString`
mOptionalInt                  `42`
mEnum                         `Bar`
mOption                       `TestOption`
mFlag                         `true`
mVerbose                      `3`
mPositional                   `Derp`
)EOF"));
}
