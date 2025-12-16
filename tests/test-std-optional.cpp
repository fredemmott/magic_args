// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>
#include "output.hpp"
#include "arg-type-definitions.hpp"

TEST_CASE("std::optional") {
  std::vector<std::string_view> argv {"my_test"};
  Output out, err;
  auto args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK_FALSE(args->mValue.has_value());

  argv.push_back("--value=");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mValue.has_value());
  CHECK(args->mValue.value() == "");

  argv.push_back("--value=foo");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mValue.has_value());
  CHECK(args->mValue.value() == "foo");
}

TEST_CASE("option<std::optional>") {
  std::vector<std::string_view> argv {"my_test"};
  Output out, err;
  auto args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK_FALSE(args->mDocumentedValue.has_value());

  argv.push_back("--documented-value=");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mDocumentedValue.has_value());
  CHECK(args->mDocumentedValue.value() == "");

  argv.push_back("--documented-value=foo");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mDocumentedValue.has_value());
  CHECK(args->mDocumentedValue.value() == "foo");

  // Check it's a mutable reference
  *args->mDocumentedValue = "bar";
  CHECK(args->mDocumentedValue.value() == "bar");
}

TEST_CASE("optional_positional_argument<std::optional>") {
  std::vector<std::string_view> argv {"my_test"};
  Output out, err;
  auto args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK_FALSE(args->mPositional.has_value());

  argv.emplace_back("");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  REQUIRE(args->mPositional.has_value());
  CHECK(args->mPositional.value() == "");

  argv.pop_back();
  argv.push_back("foo");
  CHECK(args->mPositional.value() == "");
  args = magic_args::parse<Optional>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  REQUIRE(args->mPositional.has_value());
  CHECK(args->mPositional.value() == "foo");
}
