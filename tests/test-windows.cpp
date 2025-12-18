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

#include <algorithm>
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

std::optional<DWORD> GetEncodingError(
  const magic_args::make_utf8_argv_error_t& e) {
  const auto p = get_if<magic_args::encoding_conversion_failed_t>(&e);
  if (!p) {
    return std::nullopt;
  }
  if (const auto code = p->mPlatformErrorCode.value();
      std::in_range<DWORD>(code)) {
    return static_cast<DWORD>(code);
  }
  return std::nullopt;
}

template <>
struct Catch::StringMaker<std::filesystem::path> {
  static std::string convert(const std::filesystem::path& value) {
    return value.string();
  }
};

struct PathIsEquivalent : MatcherBase<std::filesystem::path> {
  PathIsEquivalent() = delete;
  explicit PathIsEquivalent(std::filesystem::path expected)
    : mExpected(std::move(expected)) {
  }
  bool match(const std::filesystem::path& actual) const override {
    std::error_code ec;
    return std::filesystem::equivalent(actual, mExpected, ec);
  }

  std::string describe() const override {
    return std::format(
      "is equivalent to {}", Catch::Detail::stringify(mExpected));
  }

 private:
  std::filesystem::path mExpected;
};

TEST_CASE("wWinMain", "[windows]") {
  // wWinMain gives us the command line all in one UTF-16 string
  constexpr auto commandLine
    = L"test_app --foo 💩 --bar \"Dzień dobry\" --empty \"\" --baz test";
  const auto argv = magic_args::make_utf8_argv(commandLine);
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
  const auto argv = magic_args::make_utf8_argv(commandLine);
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

TEST_CASE("invalid UTF-16", "[windows]") {
  constexpr auto commandLine
    = L"test_app --foo abc"
      L"\xD800"
      L"def";
  const auto argv = magic_args::make_utf8_argv(commandLine);
  REQUIRE_FALSE(argv.has_value());
  CHECK(GetEncodingError(argv.error()) == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("valid characters in legacy code page", "[windows]") {
  constexpr auto commandLine = "testApp --foo \xa7\x41\xa6\x6e";

  const auto asBig5 = magic_args::make_utf8_argv(commandLine, 950);
  CHECK(asBig5.has_value());
  if (asBig5.has_value()) {
    CHECK_THAT(
      *asBig5,
      RangeEquals({
        "testApp",
        "--foo",
        "你好",
      }));
  }

  const auto asUtf8 = magic_args::make_utf8_argv(commandLine, CP_UTF8);
  REQUIRE_FALSE(asUtf8.has_value());
  CHECK(GetEncodingError(asUtf8.error()) == ERROR_NO_UNICODE_TRANSLATION);
}

TEST_CASE("empty string", "[windows]") {
  // Unicode paths on Windows can be larger than MAX_PATH
  wchar_t thisExeBuffer[32768];
  const std::wstring_view thisExeWide {
    thisExeBuffer,
    GetModuleFileNameW(
      nullptr, thisExeBuffer, static_cast<DWORD>(std::size(thisExeBuffer))),
  };
  const auto thisExe = [thisExeWide] {
    std::string buffer;
    REQUIRE(magic_args::detail::win32::utf8_from_wide(buffer, thisExeWide));
    return std::filesystem::path {buffer};
  }();

  const auto narrow = magic_args::make_utf8_argv("");

  CHECK(narrow);
  if (narrow) {
    CHECK_FALSE(narrow->empty());
    if (!narrow->empty()) {
      CHECK_THAT(narrow->front(), PathIsEquivalent(thisExe));
    }
  }
  const auto wide = magic_args::make_utf8_argv(L"");
  CHECK(wide);
  if (wide) {
    CHECK_FALSE(wide->empty());
    if (!wide->empty()) {
      CHECK_THAT(wide->front(), PathIsEquivalent(thisExe));
    }
  }

  const auto null = magic_args::make_utf8_argv(nullptr);
  CHECK_FALSE(null.has_value());
  if (!null.has_value()) {
    CHECK(holds_alternative<magic_args::invalid_parameter_t>(null.error()));
  }
}