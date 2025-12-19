// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifdef TEST_SINGLE_HEADER
#define MAGIC_ARGS_ENABLE_SUBCOMMANDS
#include <magic_args/magic_args.hpp>
#else
#include <magic_args/subcommands.hpp>
#endif

#include <format>
#include <string>

namespace TestSubcommands {
struct CommandFooBar {
  static constexpr auto name = "foo";
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
    constexpr bool operator==(const arguments_type&) const noexcept = default;
  };

  static std::string main(arguments_type&& args) {
    return std::format(
      "TEST RESULT CommandFooBar --bar={} --baz={}", args.mBar, args.mBaz);
  }
};

struct CommandHerp {
  static constexpr auto name = "herp";

  struct arguments_type {
    static constexpr auto description = "Description goes here";
    static constexpr auto version = "Version goes here";

    std::string mDerp;
    constexpr bool operator==(const arguments_type&) const noexcept = default;
  };

  static std::string main(arguments_type&& args) {
    return std::format("TEST RESULT CommandHerp --derp={}", args.mDerp);
  }
};

template <magic_args::invocable_subcommand Parent>
struct CommandReturnsVoid : Parent {
  static inline std::optional<typename Parent::arguments_type> invocation {};
  static void main(typename Parent::arguments_type&& args) {
    invocation.emplace(args);
    std::ignore = Parent::main(std::forward<decltype(args)>(args));
  }
};

}// namespace TestSubcommands
