// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "arg-type-definitions.hpp"
#include "chomp.hpp"
#include "output.hpp"

using namespace Catch::Matchers;

constexpr char testName[] = "C:/Foo/Bar/my_test.exe";

TEST_CASE("basic_argument concept") {
  using namespace magic_args::public_api;

  // Options are `basic_argument`'s but not positional arguments
  STATIC_CHECK(basic_argument<flag>);
  STATIC_CHECK(basic_argument<counted_flag>);
  STATIC_CHECK(basic_argument<option<std::string>>);

  STATIC_CHECK(basic_argument<optional_positional_argument<std::string>>);
  STATIC_CHECK(basic_argument<mandatory_positional_argument<std::string>>);
}

TEST_CASE("basic_option concept") {
  using namespace magic_args::public_api;

  STATIC_CHECK(basic_option<flag>);
  STATIC_CHECK(basic_option<counted_flag>);
  STATIC_CHECK(basic_option<option<std::string>>);

  STATIC_CHECK_FALSE(basic_option<optional_positional_argument<std::string>>);
  STATIC_CHECK_FALSE(basic_option<mandatory_positional_argument<std::string>>);
}

TEST_CASE("basic_positional_argument concept") {
  using namespace magic_args::public_api;
  STATIC_CHECK_FALSE(basic_positional_argument<flag>);
  STATIC_CHECK_FALSE(basic_positional_argument<counted_flag>);
  STATIC_CHECK_FALSE(basic_positional_argument<option<std::string>>);

  STATIC_CHECK(
    basic_positional_argument<optional_positional_argument<std::string>>);
  STATIC_CHECK(
    basic_positional_argument<mandatory_positional_argument<std::string>>);
}

TEST_CASE("header build mode") {
#if __has_include(<magic_args/parse.hpp>)
  constexpr bool haveParseHpp = true;
#else
  constexpr bool haveParseHpp = false;
#endif

#ifdef TEST_SINGLE_HEADER
  STATIC_CHECK(magic_args::is_single_header_file);
  STATIC_CHECK_FALSE(haveParseHpp);
#else
  STATIC_CHECK_FALSE(magic_args::is_single_header_file);
  STATIC_CHECK(haveParseHpp);
#endif
}

TEMPLATE_TEST_CASE(
  "no args",
  "",
  EmptyStruct,
  OptionsOnly,
  FlagsOnly,
  FlagsAndPositionalArguments,
  MultiValuePositionalArgument) {
  std::vector<std::string_view> argv {testName};
  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  CHECK(args.has_value());
  STATIC_CHECK(std::same_as<const TestType&, decltype(*args)>);
  CHECK(out.empty());
  CHECK(err.empty());
}

TEMPLATE_TEST_CASE(
  "bogus flag after --",
  "",
  EmptyStruct,
  OptionsOnly,
  FlagsOnly) {
  std::vector<std::string_view> argv {testName, "--", "--not-a-valid-arg"};
  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: Unexpected argument: --not-a-valid-arg

Usage: my_test [OPTIONS...]
)EOF")}));
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::invalid_argument>(args.error()));
}

TEMPLATE_TEST_CASE(
  "bogus flag after -- (silent)",
  "",
  EmptyStruct,
  OptionsOnly,
  FlagsOnly) {
  std::vector<std::string_view> argv {testName, "--", "--not-a-valid-arg"};
  const auto args = magic_args::parse_silent<TestType>(argv);
  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument>(args.error()));
  const auto& e = get<magic_args::invalid_argument>(args.error());
  // Positional because of `--`
  CHECK(e.mKind == magic_args::invalid_argument::kind::Positional);
  CHECK(e.mSource.mArg == "--not-a-valid-arg");
}

struct EmptyWithVersion {
  static constexpr auto version = "MyApp v1.2.3";
};

TEST_CASE("empty struct, --version") {
  std::vector<std::string_view> argv {testName, "--version"};

  Output out, err;
  const auto args = magic_args::parse<EmptyWithVersion>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::version_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == "MyApp v1.2.3\n");
}

TEMPLATE_TEST_CASE(
  "empty struct, invalid argument",
  "",
  EmptyStruct,
  FlagsOnly) {
  const auto invalid = GENERATE("--abc", "-z");
  std::vector<std::string_view> argv {testName, invalid};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::invalid_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(
    err.get(),
    StartsWith(
      std::format(
        R"EOF(
my_test: Unrecognized option: {}

Usage: my_test [OPTIONS...]
)EOF",
        invalid)
        .substr(1)));
}

TEMPLATE_TEST_CASE(
  "empty struct, invalid argument (silent)",
  "",
  EmptyStruct,
  FlagsOnly) {
  const auto invalid = GENERATE("--abc", "-z");
  std::vector<std::string_view> argv {testName, invalid};

  const auto args = magic_args::parse_silent<TestType>(argv);
  REQUIRE_FALSE(args.has_value());
  REQUIRE(std::holds_alternative<magic_args::invalid_argument>(args.error()));
  const auto& e = get<magic_args::invalid_argument>(args.error());
  CHECK(e.mKind == magic_args::invalid_argument::kind::Option);
  CHECK(e.mSource.mArg == invalid);
}

TEST_CASE("multiple short flags") {
  const auto args
    = magic_args::parse_silent<ShortFlags>(std::array {"test", "-ac"});
  REQUIRE(args.has_value());
  CHECK(args->mFlagA);
  CHECK_FALSE(args->mFlagB);
  CHECK(args->mFlagC);
}

TEST_CASE("flags only, specifying flags") {
  std::vector<std::string_view> argv {testName, "--foo"};

  Output out, err;

  auto args = magic_args::parse<FlagsOnly>(argv, out, err);
  REQUIRE(args.has_value());
  CHECK(args->mFoo);
  CHECK_FALSE(args->mBar);
  CHECK_FALSE(args->mBaz);

  argv.push_back("--bar");
  args = magic_args::parse<FlagsOnly>(argv, out, err);
  CHECK(args->mFoo);
  CHECK(args->mBar);
  CHECK_FALSE(args->mBaz);

  argv.push_back("--baz");
  args = magic_args::parse<FlagsOnly>(argv, out, err);
  CHECK(args->mFoo);
  CHECK(args->mBar);
  CHECK(args->mBaz);

  argv = {testName, "--baz"};
  args = magic_args::parse<FlagsOnly>(argv, out, err);
  CHECK_FALSE(args->mFoo);
  CHECK_FALSE(args->mBar);
  CHECK(args->mBaz);

  CHECK(out.empty());
  CHECK(err.empty());
}

TEST_CASE("options only, all provided, --foo value") {
  std::vector<std::string_view> argv {
    testName,
    "--string",
    "value",
    "--int",
    "123",
    "--foo",
    "abc",
  };
  Output out, err;
  const auto args = magic_args::parse<OptionsOnly>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mString == "value");
  CHECK(args->mInt == 123);
  CHECK(args->mDocumentedString == "abc");
}

TEST_CASE("options only, --foo=value") {
  std::vector<std::string_view> argv {testName, "--foo=abc"};
  Output out, err;
  const auto args = magic_args::parse<OptionsOnly>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mDocumentedString == "abc");
}

TEST_CASE("options only, short") {
  std::vector<std::string_view> argv {
    testName,
    "-f",
    "abc",
  };
  Output out, err;
  const auto args = magic_args::parse<OptionsOnly>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mDocumentedString == "abc");
}

TEMPLATE_TEST_CASE(
  "parameters, all provided",
  "",
  FlagsAndPositionalArguments,
  MandatoryPositionalArgument) {
  std::vector<std::string_view> argv {testName, "in", "out"};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  REQUIRE(args.has_value());
  CHECK(out.empty());
  CHECK(err.empty());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mInput == "in");
  CHECK(args->mOutput == "out");
}

TEST_CASE("parameters, omitted optional") {
  std::vector<std::string_view> argv {testName, "in"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, out, err);
  REQUIRE(args.has_value());
  CHECK(out.empty());
  CHECK(err.empty());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mInput == "in");
  CHECK(args->mOutput.mValue.empty());
}

TEST_CASE("parameters, extra") {
  std::vector<std::string_view> argv {testName, "in", "out", "bogus"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::invalid_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: Unexpected argument: bogus

Usage: my_test [OPTIONS...] [--] [INPUT] [OUTPUT]
)EOF")}));
}

TEST_CASE("positional parameters with flag as value") {
  std::vector<std::string_view> argv {testName, "in", "--", "--flag"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());

  REQUIRE(args.has_value());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mInput == "in");
  CHECK(args->mOutput == "--flag");
}

TEST_CASE("missing mandatory named parameter") {
  std::vector<std::string_view> argv {
    testName,
  };

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryPositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: Missing required argument `INPUT`

Usage: my_test [OPTIONS...] [--] INPUT [OUTPUT]
)EOF")}));
}

TEMPLATE_TEST_CASE(
  "multi-value named argument, all specified",
  "",
  MultiValuePositionalArgument,
  MandatoryMultiValuePositionalArgument) {
  std::vector<std::string_view> argv {testName, "out", "in"};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  REQUIRE(args.has_value());
  CHECK(err.empty());
  CHECK(out.empty());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mOutput == "out");
  CHECK(args->mInputs == std::vector<std::string> {"in"});
}

TEMPLATE_TEST_CASE(
  "multi-value named argument, multiple specified",
  "",
  MultiValuePositionalArgument,
  MandatoryMultiValuePositionalArgument) {
  std::vector<std::string_view> argv {testName, "out", "in1", "in2"};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, out, err);
  REQUIRE(args.has_value());
  CHECK(err.empty());
  CHECK(out.empty());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mOutput == "out");
  CHECK(args->mInputs == std::vector<std::string> {"in1", "in2"});
}

TEST_CASE("mandatory multi-value named argument, missing all") {
  std::vector<std::string_view> argv {testName, "--flag"};

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryMultiValuePositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: Missing required argument `OUTPUT`

Usage: my_test [OPTIONS...] [--] OUTPUT INPUT [INPUT [...]]
)EOF")}));
}

TEST_CASE("mandatory multi-value named argument, missing first") {
  std::vector<std::string_view> argv {testName, "--flag", "OUTPUT"};

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryMultiValuePositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: Missing required argument `INPUTS`

Usage: my_test [OPTIONS...] [--] OUTPUT INPUT [INPUT [...]]
)EOF")}));
}

TEST_CASE("custom arguments") {
  std::vector<std::string_view> argv {
    testName, "--raw=123", "--option=456", "789"};

  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.get() == "");
  REQUIRE(args.has_value());
  CHECK(args->mRaw.mValue == "123");
  CHECK(args->mOption.mValue.mValue == "456");
  CHECK(args->mPositional.mValue.mValue == "789");
}

TEST_CASE("invalid value") {
  constexpr std::string_view argv[] {
    testName, "--raw", MyValueType::InvalidValue};

  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `--raw` (seen: `--raw ___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [POSITIONAL]
)EOF")}));

  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "--raw");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("positional argument with custom type") {
  constexpr std::string_view argv[] {testName, "ABC"};

  Output out, err;
  const auto args = magic_args::parse<CustomPositionalArgument>(argv, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mFoo.mValue.mValue == "ABC");
}

TEST_CASE("invalid value for positional argument") {
  constexpr std::string_view argv[] {testName, MyValueType::InvalidValue};

  Output out, err;
  const auto args = magic_args::parse<CustomPositionalArgument>(argv, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `FOO` (seen: `___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [FOO]
)EOF")}));

  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "FOO");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("missing argument value") {
  constexpr std::string_view argv[] {testName, "--raw"};
  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, out, err);
  CHECK(out.empty());
  CHECK_THAT(err.get(), StartsWith(std::string {chomp(R"EOF(
my_test: option `--raw` requires a value

Usage: my_test [OPTIONS...] [--] [POSITIONAL]
)EOF")}));
  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::missing_argument_value>(args.error()));
  const auto& e = get<magic_args::missing_argument_value>(args.error());
  CHECK(e.mSource.mName == "--raw");
}