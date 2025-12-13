// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <ranges>

#include "output.hpp"

struct EmptyStruct {};

namespace MyNS {
struct MyValueType {
  static constexpr std::string_view InvalidValue {"___MAGIC_INVALID___"};
  std::string mValue;
};
std::expected<void, magic_args::invalid_argument_value> from_string_argument(
  MyValueType& v,
  std::string_view arg) {
  if (arg == MyValueType::InvalidValue) {
    return std::unexpected {magic_args::invalid_argument_value {}};
  }
  v.mValue = std::string {arg};
  return {};
}
}// namespace MyNS
using MyNS::MyValueType;

struct CustomArgs {
  MyValueType mRaw;
  magic_args::option<MyValueType> mOption {
    .mHelp = "std::optional",
  };
  magic_args::optional_positional_argument<MyValueType> mPositional;
};

struct Normalization {
  std::string mEmUpperCamel;
  std::string m_EmUnderscoreUpperCamel;
  std::string _UnderscoreUpperCamel;
  std::string _underscoreLowerCamel;
  std::string UpperCamel;
  std::string lowerCamel;
  std::string m_em_snake_case;
  std::string snake_case;
};

struct Optional {
  std::optional<std::string> mValue;
  magic_args::option<std::optional<std::string>> mDocumentedValue {
    .mHelp = "documented value",
  };
  magic_args::optional_positional_argument<std::optional<std::string>>
    mPositional {
      .mHelp = "absent != empty",
    };
};

struct FlagsOnly {
  bool mFoo {false};
  bool mBar {false};
  magic_args::flag mBaz {
    "baz",
    "do the bazzy thing",
    "b",
  };

  bool operator==(const FlagsOnly&) const noexcept = default;
};

struct OptionsOnly {
  std::string mString;
  int mInt {0};
  magic_args::option<std::string> mDocumentedString {
    {},
    "foo",
    "do the foo thing",
    "f",
  };

  bool operator==(const OptionsOnly&) const noexcept = default;
};

struct FlagsAndPositionalArguments {
  bool mFlag {false};
  magic_args::optional_positional_argument<std::string> mInput;
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
};

struct MandatoryPositionalArgument {
  bool mFlag {false};
  magic_args::mandatory_positional_argument<std::string> mInput;
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
};

struct MultiValuePositionalArgument {
  bool mFlag {false};
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
  magic_args::optional_positional_argument<std::vector<std::string>> mInputs;
};

struct MandatoryMultiValuePositionalArgument {
  bool mFlag {false};
  magic_args::mandatory_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
  magic_args::mandatory_positional_argument<std::vector<std::string>> mInputs;
};

struct CustomPositionalArgument {
  magic_args::optional_positional_argument<MyValueType> mFoo;
};

constexpr char testName[] = "C:/Foo/Bar/my_test.exe";

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
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
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
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK_THAT(err.get(), Catch::Matchers::StartsWith(&R"EOF(
my_test: Unexpected argument: --not-a-valid-arg

Usage: my_test [OPTIONS...]
)EOF"[1]));
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
  const auto args = magic_args::parse_silent<TestType>(argv, {});
  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument>(args.error()));
  const auto& e = get<magic_args::invalid_argument>(args.error());
  // Positional because of `--`
  CHECK(e.mKind == magic_args::invalid_argument::kind::Positional);
  CHECK(e.mSource.mArg == "--not-a-valid-arg");
}

TEST_CASE("empty struct, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<EmptyStruct>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::invalid_argument>(args.error()));
  CHECK(out.empty());
  CHECK_THAT(
    err.get(),
    Catch::Matchers::StartsWith(
      std::format(
        R"EOF(
my_test: Unrecognized option: {}

Usage: my_test [OPTIONS...]

Options:
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

  const auto args = magic_args::parse_silent<TestType>(argv, {});
  REQUIRE_FALSE(args.has_value());
  REQUIRE(std::holds_alternative<magic_args::invalid_argument>(args.error()));
  const auto& e = get<magic_args::invalid_argument>(args.error());
  CHECK(e.mKind == magic_args::invalid_argument::kind::Option);
  CHECK(e.mSource.mArg == invalid);
}

TEST_CASE("flags only, specifying flags") {
  std::vector<std::string_view> argv {testName, "--foo"};

  Output out, err;

  auto args = magic_args::parse<FlagsOnly>(argv, {}, out, err);
  REQUIRE(args.has_value());
  CHECK(args->mFoo);
  CHECK_FALSE(args->mBar);
  CHECK_FALSE(args->mBaz);

  argv.push_back("--bar");
  args = magic_args::parse<FlagsOnly>(argv, {}, out, err);
  CHECK(args->mFoo);
  CHECK(args->mBar);
  CHECK_FALSE(args->mBaz);

  argv.push_back("--baz");
  args = magic_args::parse<FlagsOnly>(argv, {}, out, err);
  CHECK(args->mFoo);
  CHECK(args->mBar);
  CHECK(args->mBaz);

  argv = {testName, "--baz"};
  args = magic_args::parse<FlagsOnly>(argv, {}, out, err);
  CHECK_FALSE(args->mFoo);
  CHECK_FALSE(args->mBar);
  CHECK(args->mBaz);

  CHECK(out.empty());
  CHECK(err.empty());
}

TEST_CASE("flags only, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<FlagsOnly>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --foo
      --bar
  -b, --baz                    do the bazzy thing

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("options only, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<OptionsOnly>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: my_test [OPTIONS...]

Options:

      --string=VALUE
      --int=VALUE
  -f, --foo=VALUE              do the foo thing

  -?, --help                   show this message
)EOF"[1]);
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
  const auto args = magic_args::parse<OptionsOnly>(argv, {}, out, err);
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
  const auto args = magic_args::parse<OptionsOnly>(argv, {}, out, err);
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
  const auto args = magic_args::parse<OptionsOnly>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mDocumentedString == "abc");
}

TEST_CASE("parameters, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));

  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: my_test [OPTIONS...] [--] [INPUT] [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"[1]);
}

TEMPLATE_TEST_CASE(
  "parameters, all provided",
  "",
  FlagsAndPositionalArguments,
  MandatoryPositionalArgument) {
  std::vector<std::string_view> argv {testName, "in", "out"};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
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
    = magic_args::parse<FlagsAndPositionalArguments>(argv, {}, out, err);
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
    = magic_args::parse<FlagsAndPositionalArguments>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::invalid_argument>(args.error()));
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: Unexpected argument: bogus

Usage: my_test [OPTIONS...] [--] [INPUT] [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"[1]);
}

TEST_CASE("positional parameters with flag as value") {
  std::vector<std::string_view> argv {testName, "in", "--", "--flag"};

  Output out, err;
  const auto args
    = magic_args::parse<FlagsAndPositionalArguments>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());

  REQUIRE(args.has_value());
  CHECK_FALSE(args->mFlag);
  CHECK(args->mInput == "in");
  CHECK(args->mOutput == "--flag");
}

TEST_CASE("mandatory named parameter, --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryPositionalArgument>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: my_test [OPTIONS...] [--] INPUT [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"[1]);
}

TEST_CASE("missing mandatory named parameter") {
  std::vector<std::string_view> argv {
    testName,
  };

  Output out, err;
  const auto args
    = magic_args::parse<MandatoryPositionalArgument>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: Missing required argument `INPUT`

Usage: my_test [OPTIONS...] [--] INPUT [OUTPUT]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      INPUT
      OUTPUT                   file to create
)EOF"[1]);
}

TEST_CASE("multi-value parameter - --help") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args
    = magic_args::parse<MultiValuePositionalArgument>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: my_test [OPTIONS...] [--] [OUTPUT] [INPUT [INPUT [...]]]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      OUTPUT                   file to create
      INPUTS
)EOF"[1]);
}

TEMPLATE_TEST_CASE(
  "multi-value named argument, all specified",
  "",
  MultiValuePositionalArgument,
  MandatoryMultiValuePositionalArgument) {
  std::vector<std::string_view> argv {testName, "out", "in"};

  Output out, err;
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
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
  const auto args = magic_args::parse<TestType>(argv, {}, out, err);
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
  const auto args = magic_args::parse<MandatoryMultiValuePositionalArgument>(
    argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: Missing required argument `OUTPUT`

Usage: my_test [OPTIONS...] [--] OUTPUT INPUT [INPUT [...]]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      OUTPUT                   file to create
      INPUTS
)EOF"[1]);
}

TEST_CASE("mandatory multi-value named argument, missing first") {
  std::vector<std::string_view> argv {testName, "--flag", "OUTPUT"};

  Output out, err;
  const auto args = magic_args::parse<MandatoryMultiValuePositionalArgument>(
    argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::missing_required_argument>(args.error()));
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: Missing required argument `INPUTS`

Usage: my_test [OPTIONS...] [--] OUTPUT INPUT [INPUT [...]]

Options:

      --flag

  -?, --help                   show this message

Arguments:

      OUTPUT                   file to create
      INPUTS
)EOF"[1]);
}

TEST_CASE("std::optional") {
  std::vector<std::string_view> argv {testName};
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
  std::vector<std::string_view> argv {testName};
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
  std::vector<std::string_view> argv {testName};
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

TEST_CASE("custom arguments") {
  std::vector<std::string_view> argv {
    testName, "--raw=123", "--option=456", "789"};

  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.get() == "");
  REQUIRE(args.has_value());
  CHECK(args->mRaw.mValue == "123");
  CHECK(args->mOption.mValue.mValue == "456");
  CHECK(args->mPositional.mValue.mValue == "789");
}

TEST_CASE("GNU-style normalization") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<Normalization>(argv, {}, out, err);
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
)EOF"[1]);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
}

TEST_CASE("PowerShell-style normalization") {
  std::vector<std::string_view> argv {testName, "-Help"};

  Output out, err;
  const auto args = magic_args::
    parse<Normalization, magic_args::powershell_style_parsing_traits>(
      argv, {}, out, err);
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
)EOF"[1]);
  REQUIRE_FALSE(args.has_value());
  CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
}

TEST_CASE("GNU-style verbatim names") {
  std::vector<std::string_view> argv {testName, "--help"};

  Output out, err;
  const auto args = magic_args::parse<
    Normalization,
    magic_args::verbatim_names<magic_args::gnu_style_parsing_traits>>(
    argv, {}, out, err);
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
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
)EOF"[1]);
}

TEST_CASE("PowerShell-style verbatim names") {
  std::vector<std::string_view> argv {testName, "-Help"};

  Output out, err;
  const auto args = magic_args::parse<
    Normalization,
    magic_args::verbatim_names<magic_args::powershell_style_parsing_traits>>(
    argv, {}, out, err);
  CHECK(err.get() == "");
  CHECK(out.get() == &R"EOF(
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
)EOF"[1]);
}

TEST_CASE("GNU-style invalid value") {
  constexpr std::string_view argv[] {
    testName, "--raw", MyValueType::InvalidValue};

  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `--raw` (seen: `--raw ___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [POSITIONAL]

Options:

      --raw=VALUE
      --option=VALUE           std::optional

  -?, --help                   show this message

Arguments:

      POSITIONAL
)EOF"[1]);

  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "--raw");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("PowerShell-style invalid value") {
  constexpr std::string_view argv[] {
    testName, "-Raw", MyValueType::InvalidValue};

  Output out, err;
  const auto args = magic_args::
    parse<CustomArgs, magic_args::powershell_style_parsing_traits>(
      argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `-Raw` (seen: `-Raw ___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [POSITIONAL]

Options:

      -Raw=VALUE
      -Option=VALUE            std::optional

  -?, -Help                    show this message

Arguments:

      POSITIONAL
)EOF"[1]);

  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "-Raw");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("positional argument with custom type") {
  constexpr std::string_view argv[] {testName, "ABC"};

  Output out, err;
  const auto args
    = magic_args::parse<CustomPositionalArgument>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mFoo.mValue.mValue == "ABC");
}

TEST_CASE("invalid value for positional argument") {
  constexpr std::string_view argv[] {testName, MyValueType::InvalidValue};

  Output out, err;
  const auto args
    = magic_args::parse<CustomPositionalArgument>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(out.empty());
  CHECK(err.get() == &R"EOF(
my_test: `___MAGIC_INVALID___` is not a valid value for `FOO` (seen: `___MAGIC_INVALID___`)

Usage: my_test [OPTIONS...] [--] [FOO]

Options:

  -?, --help                   show this message

Arguments:

      FOO
)EOF"[1]);

  REQUIRE(holds_alternative<magic_args::invalid_argument_value>(args.error()));
  const auto& e = get<magic_args::invalid_argument_value>(args.error());
  CHECK(e.mSource.mName == "FOO");
  CHECK(e.mSource.mValue == MyValueType::InvalidValue);
}

TEST_CASE("missing argument value") {
  constexpr std::string_view argv[] {testName, "--raw"};
  Output out, err;
  const auto args = magic_args::parse<CustomArgs>(argv, {}, out, err);
  CHECK(out.empty());
  CHECK_THAT(err.get(), Catch::Matchers::StartsWith(&R"EOF(
my_test: option `--raw` requires a value

Usage: my_test [OPTIONS...] [--] [POSITIONAL]
)EOF"[1]));
  REQUIRE_FALSE(args.has_value());
  REQUIRE(holds_alternative<magic_args::missing_argument_value>(args.error()));
  const auto& e = get<magic_args::missing_argument_value>(args.error());
  CHECK(e.mSource.mName == "--raw");
}