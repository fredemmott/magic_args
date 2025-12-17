// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifdef TEST_SINGLE_HEADER
#ifdef _WIN32
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#else
#define MAGIC_ARGS_ENABLE_ICONV_EXTENSIONS
#endif

#include <magic_args/magic_args.hpp>
#else
#include <magic_args/magic_args.hpp>
#ifdef _WIN32
#include <magic_args/windows.hpp>
#else
#include <magic_args/iconv.hpp>
#endif
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#ifdef CATCH_CONFIG_FAST_COMPILE
#error "BOO"
#endif

#include <string>

struct MyArgs {
  std::string mFoo;
  std::string mBar;
  std::string mBaz;
  std::string mEmpty;
};
using namespace Catch::Matchers;

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

auto make_utf8_argv(
  int argc,
  const char* const* argv,
  const unsigned int codepage) {
#ifdef _WIN32
  return magic_args::make_utf8_argv(argc, argv, codepage);
#else
  if (codepage == CP_UTF8) {
    return magic_args::make_utf8_argv(argc, argv, "UTF-8");
  } else {
    return magic_args::make_utf8_argv(
      argc, argv, std::format("CP{}", codepage));
  }
#endif
}

auto make_utf8_argv(const auto& argv, const unsigned int codepage) {
  return make_utf8_argv(static_cast<int>(argv.size()), argv.data(), codepage);
}

#ifdef MAGIC_ARGS_HAVE_ICONV_EXTENSIONS
constexpr int InvalidBytes = EILSEQ;
#endif
#ifdef MAGIC_ARGS_HAVE_WINDOWS_EXTENSIONS
constexpr int InvalidBytes = ERROR_NO_UNICODE_TRANSLATION;
#endif

[[nodiscard]]
static bool IsInvalidByteSequence(const magic_args::make_utf8_argv_error_t& e) {
  const auto p = get_if<magic_args::encoding_conversion_failed_t>(&e);
  if (!p) {
    return false;
  }
  return p->mPlatformErrorCode.value() == InvalidBytes;
}

TEST_CASE("invalid UTF-8") {
  constexpr std::array in {
    "test_app",
    "--foo",
    "abc"
    "\x80"
    "def",
  };
  const auto out = make_utf8_argv(in, CP_UTF8);
  REQUIRE_FALSE(out.has_value());
  REQUIRE(
    holds_alternative<magic_args::encoding_conversion_failed_t>(out.error()));
}

TEST_CASE("invalid character in legacy code page") {
  constexpr std::array argv {"testApp", "--foo", "abc€def"};
  const auto asBig5 = make_utf8_argv(argv, 950);
  CHECK_FALSE(asBig5);
  if (!asBig5) {
    CHECK(
      holds_alternative<magic_args::encoding_conversion_failed_t>(
        asBig5.error()));
    CHECK(IsInvalidByteSequence(asBig5.error()));
  }
  const auto asUtf8 = make_utf8_argv(argv, CP_UTF8);
  CHECK(asUtf8);
  if (asUtf8) {
    CHECK_THAT(
      *asUtf8,
      RangeEquals({
        "testApp",
        "--foo",
        "abc€def",
      }));
  }
}

TEST_CASE("valid characters in legacy code page") {
  constexpr std::array argv {"testApp", "--foo", "\xa7\x41\xa6\x6e"};

  const auto asBig5 = make_utf8_argv(argv, 950);
  CHECK(asBig5.has_value());
  if (asBig5) {
    CHECK_THAT(
      *asBig5,
      RangeEquals({
        "testApp",
        "--foo",
        "你好",
      }));
  }

  const auto asUtf8 = make_utf8_argv(argv, CP_UTF8);
  CHECK_FALSE(asUtf8);
  if (!asUtf8) {
    CHECK(
      holds_alternative<magic_args::encoding_conversion_failed_t>(
        asUtf8.error()));
    CHECK(IsInvalidByteSequence(asUtf8.error()));
  }
}