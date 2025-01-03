// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include "magic_args/magic_args.hpp"

struct MyArgs {
  std::string mFoo;
  int mBar;
  magic_args::option<std::string> mBaz {
    .mHelp = "do the bazzy thing",
    .mShortName = "b",
  };
  bool mFlag {false};
  bool mOtherFlag {false};
  magic_args::mandatory_positional_argument<std::string> mOutput;
  magic_args::optional_positional_argument<std::vector<std::string>> mInputs;
};

int main(int argc, char** argv) {
  const auto args = magic_args::parse<MyArgs>(argc, argv);
  if (!args) {
    using enum magic_args::incomplete_parse_reason;
    switch (args.error()) {
      case HelpRequested:
      case VersionRequested:
        return EXIT_SUCCESS;
      default:
        return EXIT_FAILURE;
    }
  }

  static_assert(std::same_as<MyArgs, std::decay_t<decltype(*args)>>);
  std::println("mFlag: {}", args->mFlag ? "true" : "false");
  std::println("\nGenerated dump:");
  magic_args::dump(*args);

  return 0;
}
