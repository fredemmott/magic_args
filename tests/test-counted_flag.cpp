// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "test_output.hpp"

struct MyArgs {
  magic_args::counted_flag mVerbose {
    .short_name = "v",
  };
};

TEST_CASE("empty") {
  const auto args = magic_args::parse_silent<MyArgs>(std::array {"myapp"});
  REQUIRE(args.has_value());
  CHECK_FALSE(args->mVerbose);
  CHECK(args->mVerbose.storage == 0);
}

TEST_CASE("single") {
  const auto args
    = magic_args::parse_silent<MyArgs>(std::array {"myapp", "-v"});
  REQUIRE(args.has_value());
  CHECK(args->mVerbose);
  CHECK(args->mVerbose.storage == 1);
}

TEST_CASE("multiple") {
  const auto args
    = magic_args::parse_silent<MyArgs>(std::array {"myapp", "-v", "-v"});
  REQUIRE(args.has_value());
  CHECK(args->mVerbose);
  CHECK(args->mVerbose.storage == 2);
}

TEST_CASE("multiple in single argument") {
  const auto args
    = magic_args::parse_silent<MyArgs>(std::array {"myapp", "-vvv"});
  REQUIRE(args.has_value());
  CHECK(args->mVerbose);
  CHECK(args->mVerbose.storage == 3);
}

TEST_CASE("explicit value") {
  const auto args
    = magic_args::parse_silent<MyArgs>(std::array {"myapp", "--verbose=42"});
  REQUIRE(args.has_value());
  CHECK(args->mVerbose);
  CHECK(args->mVerbose.storage == 42);
}

TEST_CASE("mixed setters") {
  const auto args = magic_args::parse_silent<MyArgs>(
    std::array {"myapp", "-v", "--verbose=42", "-vv"});
  REQUIRE(args.has_value());
  CHECK(args->mVerbose);
  // Check that we got the `-vv` after the `--verbose` (even number), but not
  // the
  // `-v` before it (odd number)
  CHECK(args->mVerbose.storage == 44);
}

TEST_CASE("help") {
  test_output output;
  const auto args
    = magic_args::parse<MyArgs>(std::array {"myapp", "--help"}, output);
  CHECK(!args);
  if (!args) {
    CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  }
  CHECK(output.error_str().empty());
  CHECK_THAT(
    output.out_str(),
    Catch::Matchers::ContainsSubstring("-v, --verbose[=VALUE]"));
}