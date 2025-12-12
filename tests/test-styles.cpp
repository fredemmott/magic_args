// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "magic_args/powershell_style_parsing_traits.hpp"
#endif

#include "output.hpp"

namespace TestStyles {
struct MyArgs {
  std::string mString;
  bool mFlag {false};
  magic_args::flag mDocumentedFlag {
    .mHelp = "This flag is documented",
    .mShortName = "d",
  };
};
}// namespace TestStyles
using namespace TestStyles;

TEST_CASE("help, GNU-style") {
  std::vector<std::string_view> argv {"test_app", GENERATE("--help", "-?")};
  Output out, err;
  const auto args = magic_args::parse<MyArgs>(argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: test_app [OPTIONS...]

Options:

      --string=VALUE
      --flag
  -d, --documented-flag        This flag is documented

  -?, --help                   show this message
)EOF"[1]);
}

TEST_CASE("help, powershell-style") {
  std::vector<std::string_view> argv {"test_app", "-Help"};
  Output out, err;
  const auto args
    = magic_args::parse<MyArgs, magic_args::powershell_style_parsing_traits>(
      argv, {}, out, err);
  REQUIRE_FALSE(args.has_value());
  CHECK(holds_alternative<magic_args::help_requested>(args.error()));
  CHECK(err.empty());
  CHECK(out.get() == &R"EOF(
Usage: test_app [OPTIONS...]

Options:

      -String=VALUE
      -Flag
  -d, -DocumentedFlag          This flag is documented

  -?, -Help                    show this message
)EOF"[1]);
}

// not testing GNU-style here as that's tested in test.cpp

TEST_CASE("args, powershell-style") {
  std::vector<std::string_view> argv {
    "test_app",
    "-String",
    "stringValue",
    "-Flag",
    "-DocumentedFlag",
  };
  Output out, err;
  const auto args
    = magic_args::parse<MyArgs, magic_args::powershell_style_parsing_traits>(
      argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mString == "stringValue");
  CHECK(args->mFlag);
  CHECK(args->mDocumentedFlag);
}
