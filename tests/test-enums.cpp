// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>
#include "chomp.hpp"
#include "test_output.hpp"

enum CEnum {
  Foo,
  Bar,
  Baz,
};
enum class ScopedEnum {
  Herp,
  Derp,
};

struct PlainEnumArgs {
  CEnum mCEnum {};
  ScopedEnum mScopedEnum {};
};

enum class CustomizedEnum {
  Foo,
  Bar,
};
struct CustomizedEnumArgs {
  CustomizedEnum mValue {};
};

std::expected<void, magic_args::invalid_argument_value> from_argument_value(
  CustomizedEnum& v,
  const std::string_view s) {
  using enum CustomizedEnum;
  if (s == "herp") {
    v = Foo;
    return {};
  }
  if (s == "derp") {
    v = Bar;
    return {};
  }
  return std::unexpected {magic_args::invalid_argument_value {}};
}

auto to_argument_value(const CustomizedEnum e) {
  using enum CustomizedEnum;
  switch (e) {
    case Foo:
      return "herp";
    case Bar:
      return "derp";
  }
  std::unreachable();
}

TEST_CASE("defaults") {
  const auto args
    = magic_args::parse_silent<PlainEnumArgs>(std::array {"myApp"});
  REQUIRE(args.has_value());
  CHECK(args->mCEnum == CEnum::Foo);
  CHECK(args->mScopedEnum == ScopedEnum::Herp);
}

TEST_CASE("defaults - --help") {
  test_output output;
  const auto args
    = magic_args::parse<PlainEnumArgs>(std::array {"myApp", "--help"}, output);
  CHECK_FALSE(args.has_value());
  if (!args.has_value()) {
    CHECK(std::holds_alternative<magic_args::help_requested>(args.error()));
  }

  CHECK(output.error_str().empty());
  CHECK(output.out_str() == chomp(R"EOF(
Usage: myApp [OPTIONS...]

Options:

      --c-enum=VALUE           `Foo`, `Bar`, or `Baz`
                               (default: Foo)
      --scoped-enum=VALUE      `Herp` or `Derp`
                               (default: Herp)

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("valid values") {
  const auto args = magic_args::parse_silent<PlainEnumArgs>(
    std::array {"myApp", "--c-enum=Bar", "--scoped-enum=Derp"});
  REQUIRE(args.has_value());
  CHECK(args->mCEnum == CEnum::Bar);
  CHECK(args->mScopedEnum == ScopedEnum::Derp);
}

TEST_CASE("invalid value - C enum") {
  test_output output;
  const auto args = magic_args::parse<PlainEnumArgs>(
    std::array {"myApp", "--c-enum=INVALID"}, output);
  CHECK_FALSE(args.has_value());
  if (!args.has_value()) {
    CHECK(
      std::holds_alternative<magic_args::invalid_argument_value>(args.error()));
    if (std::holds_alternative<magic_args::invalid_argument_value>(
          args.error())) {
      const auto& e = get<magic_args::invalid_argument_value>(args.error());
      CHECK(e.source.name == "--c-enum");
      CHECK(e.source.value == "INVALID");
    }
  }
  CHECK(output.out_str().empty());
  CHECK(output.error_str() == chomp(R"EOF(
myApp: `INVALID` is not a valid value for `--c-enum` (seen: `--c-enum=INVALID`)

Usage: myApp [OPTIONS...]

Options:

      --c-enum=VALUE           `Foo`, `Bar`, or `Baz`
                               (default: Foo)
      --scoped-enum=VALUE      `Herp` or `Derp`
                               (default: Herp)

  -?, --help                   show this message
)EOF"));
}

TEST_CASE("enum with customized serde - help") {
  test_output output;
  const auto args = magic_args::parse<CustomizedEnumArgs>(
    std::array {"myApp", "--help"}, output);
  CHECK_FALSE(args);
  if (!args) {
    CHECK(holds_alternative<magic_args::help_requested>(args.error()));
  }
  CHECK(output.error_str().empty());
  CHECK(output.out_str() == chomp(R"EOF(
Usage: myApp [OPTIONS...]

Options:

      --value=VALUE            `herp` or `derp`
                               (default: herp)

  -?, --help                   show this message
)EOF"));
  // Checking for string 'herp' above, value Foo here
  CHECK(
    CustomizedEnumArgs {}.mValue == decltype(CustomizedEnumArgs::mValue)::Foo);
}

TEST_CASE("enum with customized serde - parsing") {
  const auto args = magic_args::parse_silent<CustomizedEnumArgs>(
    std::array {"myapp", "--value=derp"});
  REQUIRE(args.has_value());
  CHECK(args->mValue == decltype(CustomizedEnumArgs::mValue)::Bar);
}