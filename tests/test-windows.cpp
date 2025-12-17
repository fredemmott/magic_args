// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include <catch2/matchers/catch_matchers.hpp>
#ifdef TEST_SINGLE_HEADER
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#include <magic_args/magic_args.hpp>
#else
#include <magic_args/magic_args.hpp>
#include <magic_args/windows.hpp>
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <string>

#include "chomp.hpp"
#include "output.hpp"

namespace TestWindows {
struct MyArgs {
  std::string mFoo;
  std::string mBar;
  std::string mBaz;
  std::string mEmpty;
};
}// namespace TestWindows
using namespace TestWindows;
using namespace Catch::Matchers;

TEST_CASE("wWinMain", "[windows]") {
  // wWinMain gives us the command line all in one UTF-16 string
  constexpr auto commandLine
    = L"test_app --foo 💩 --bar \"Dzień dobry\" --empty \"\" --baz test";
  const auto argv = magic_args::win32::make_argv(commandLine);
  REQUIRE(argv.has_value());
  CHECK_THAT(
    *argv,
    RangeEquals({
      "test_app",
      "--foo",
      "💩",
      "--bar",
      "Dzień dobry",
      "--empty",
      "",
      "--baz",
      "test",
    }));

  Output out, err;
  const auto args = magic_args::parse<MyArgs>(*argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mFoo == "💩");
  CHECK(args->mBar == "Dzień dobry");
  CHECK(args->mEmpty.empty());
  CHECK(args->mBaz == "test");
}

TEST_CASE("winMain", "[windows]") {
  REQUIRE(GetACP() == CP_UTF8);
  // winMain gives us the command line all in one active-code-page string
  constexpr auto commandLine
    = "test_app --foo 💩 --bar \"Dzień dobry\" --empty \"\" --baz test";
  const auto argv = magic_args::win32::make_argv(commandLine);
  REQUIRE(argv.has_value());
  CHECK_THAT(
    *argv,
    RangeEquals({
      "test_app",
      "--foo",
      "💩",
      "--bar",
      "Dzień dobry",
      "--empty",
      "",
      "--baz",
      "test",
    }));

  Output out, err;
  const auto args = magic_args::parse<MyArgs>(*argv, {}, out, err);
  CHECK(out.empty());
  CHECK(err.empty());
  REQUIRE(args.has_value());
  CHECK(args->mFoo == "💩");
  CHECK(args->mBar == "Dzień dobry");
  CHECK(args->mEmpty.empty());
  CHECK(args->mBaz == "test");
}

TEST_CASE("invalid UTF-8", "[windows]") {
  REQUIRE(GetACP() == CP_UTF8);
  constexpr auto commandLine
    = "test_app --foo abc"
      "\x80"
      "def";
  const auto argv = magic_args::win32::make_argv(commandLine);
  REQUIRE_FALSE(argv.has_value());
  CHECK(argv.error() == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("invalid UTF-16", "[windows]") {
  constexpr auto commandLine
    = L"test_app --foo abc"
      L"\xD800"
      L"def";
  const auto argv = magic_args::win32::make_argv(commandLine);
  REQUIRE_FALSE(argv.has_value());
  CHECK(argv.error() == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("invalid character in legacy code page", "[windows]") {
  constexpr auto commandLine = "testApp --foo abc€def";
  const auto asBig5 = magic_args::win32::make_argv(commandLine, 950);
  REQUIRE_FALSE(asBig5.has_value());
  CHECK(asBig5.error() == ERROR_NO_UNICODE_TRANSLATION);
  const auto asUtf8 = magic_args::win32::make_argv(commandLine, CP_UTF8);
  CHECKED_IF(asUtf8.has_value()) {
    CHECK_THAT(
      *asUtf8,
      RangeEquals({
        "testApp",
        "--foo",
        "abc€def",
      }));

    CHECKED_IF(GetACP() == CP_UTF8) {
      CHECK(asUtf8 == magic_args::win32::make_argv(commandLine));
    }
  }
}

TEST_CASE("invalid character in legacy code page (argc/argv)", "[windows]") {
  constexpr std::array commandLine {"testApp", "--foo", "abc€def"};
  const auto asBig5 = magic_args::win32::make_argv(
    static_cast<int>(commandLine.size()), commandLine.data(), 950);
  REQUIRE_FALSE(asBig5.has_value());
  CHECK(asBig5.error() == ERROR_NO_UNICODE_TRANSLATION);
  const auto asUtf8 = magic_args::win32::make_argv(
    static_cast<int>(commandLine.size()), commandLine.data(), CP_UTF8);
  CHECKED_IF(asUtf8.has_value()) {
    CHECK_THAT(
      *asUtf8,
      RangeEquals({
        "testApp",
        "--foo",
        "abc€def",
      }));

    CHECKED_IF(GetACP() == CP_UTF8) {
      CHECK(
        asUtf8
        == magic_args::win32::make_argv(
          static_cast<int>(commandLine.size()), commandLine.data()));
    }
  }
}

TEST_CASE("valid characters in legacy code page", "[windows]") {
  constexpr auto commandLine = "testApp --foo \xa7\x41\xa6\x6e";

  const auto asBig5 = magic_args::win32::make_argv(commandLine, 950);
  CHECKED_IF(asBig5.has_value()) {
    CHECK_THAT(
      *asBig5,
      RangeEquals({
        "testApp",
        "--foo",
        "你好",
      }));
  }

  const auto asUtf8 = magic_args::win32::make_argv(commandLine, CP_UTF8);
  REQUIRE_FALSE(asUtf8.has_value());
  CHECK(asUtf8.error() == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("valid characters in legacy code page - argc/argv", "[windows]") {
  constexpr std::array argv {"testApp", "--foo", "\xa7\x41\xa6\x6e"};

  const auto asBig5 = magic_args::win32::make_argv(
    static_cast<int>(argv.size()), argv.data(), 950);
  CHECKED_IF(asBig5.has_value()) {
    CHECK_THAT(
      *asBig5,
      RangeEquals({
        "testApp",
        "--foo",
        "你好",
      }));
  }

  const auto asUtf8 = magic_args::win32::make_argv(
    static_cast<int>(argv.size()), argv.data(), CP_UTF8);
  REQUIRE_FALSE(asUtf8.has_value());
  CHECK(asUtf8.error() == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("empty string", "[windows]") {
  const auto narrow = magic_args::win32::make_argv("");

  CHECKED_ELSE(narrow.has_value()) {
    CHECK(narrow.error() == ERROR_INVALID_PARAMETER);
  }
  const auto wide = magic_args::win32::make_argv(L"");
  CHECKED_ELSE(wide.has_value()) {
    CHECK(wide.error() == ERROR_INVALID_PARAMETER);
  }

  const auto null = magic_args::win32::make_argv(nullptr);
  CHECKED_ELSE(null.has_value()) {
    CHECK(null.error() == ERROR_INVALID_PARAMETER);
  }
}