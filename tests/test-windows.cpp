// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#define MAGIC_ARGS_ENABLE_WINDOWS
#include <magic_args/magic_args.hpp>
#ifndef MAGIC_ARGS_SINGLE_FILE
#include <magic_args/windows.hpp>
#endif

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "output.hpp"

namespace TestWindows {
    struct MyArgs {
        std::string mFoo;
        std::string mBar;
        std::string mBaz;
        std::string mEmpty;
    };
} // namespace TestWindows
using namespace TestWindows;

TEST_CASE("wWinMain", "[windows]") {
    // wWinMain gives us the command line all in one UTF-16 string
    constexpr auto commandLine
            = L"test_app --foo 💩 --bar \"Dzień dobry\" --empty \"\" --baz test";

    Output out, err;
    const auto args = magic_args::parse<MyArgs>(commandLine, {}, out, err);
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
    // wWinMain gives us the command line all in one active-code-page string
    constexpr auto commandLine
            = "test_app --foo 💩 --bar \"Dzień dobry\" --empty \"\" --baz test";

    Output out, err;
    const auto args = magic_args::parse<MyArgs>(commandLine, {}, out, err);
    CHECK(out.empty());
    CHECK(err.get() == "");
    REQUIRE(args.has_value());
    CHECK(args->mFoo == "💩");
    CHECK(args->mBar == "Dzień dobry");
    CHECK(args->mEmpty.empty());
    CHECK(args->mBaz == "test");
}

// The intent is to check that error reporting is plumbed in, not specifically *this* error
TEST_CASE("wWinMain - invalid argument", "[windows]") {
    constexpr auto commandLine
            = L"test_app --invalid-argument";
    Output out, err;
    const auto args = magic_args::parse<MyArgs>(commandLine, {}, out, err);
    CHECK(!args.has_value());

    CHECK(out.empty());
    CHECK(err.get() == &R"EOF(
test_app: Unrecognized option: --invalid-argument

Usage: test_app [OPTIONS...]

Options:

      --foo=VALUE
      --bar=VALUE
      --baz=VALUE
      --empty=VALUE

  -?, --help                   show this message
)EOF"[1]);
}

// The intent is to check that error reporting is plumbed in, not specifically *this* error
TEST_CASE("winMain - invalid argument", "[windows]") {
    REQUIRE(GetACP() == CP_UTF8);
    constexpr auto commandLine
            = "test_app --invalid-argument";
    Output out, err;
    const auto args = magic_args::parse<MyArgs>(commandLine, {}, out, err);
    CHECK(!args.has_value());

    CHECK(out.empty());
    CHECK(err.get() == &R"EOF(
test_app: Unrecognized option: --invalid-argument

Usage: test_app [OPTIONS...]

Options:

      --foo=VALUE
      --bar=VALUE
      --baz=VALUE
      --empty=VALUE

  -?, --help                   show this message
)EOF"[1]);
}
