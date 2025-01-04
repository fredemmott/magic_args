// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

using namespace magic_args::public_api;

struct MyCustomType {
  std::string mValue;
};
void from_string_argument(MyCustomType& value, std::string_view arg) {
  value.mValue = arg;
}
std::string formattable_argument_value(const MyCustomType& value) {
  return value.mValue;
}

struct MyArgs {
  bool mFlag {false};
  std::string mString;
  option<std::string> mWithDocs {
    .mHelp = "Here's some help",
  };
  std::optional<std::string> mOptionalString;
  int mNotAString {};
  MyCustomType mCustomType;
  option<std::string> mConfiguredString {
    "default",
    "configured-string",
    "A parameter with documentation",
    "c",
  };
  option<std::optional<std::string>> mConfiguredOptionalString {
    .mValue = "default",
    .mName = "configured-optional-string",
    .mHelp = "A parameter with documentation, where empty != absent",
    .mShortName = "o",
  };
  mandatory_positional_argument<std::string> mMandatoryPositional {
    {/* default */},
    "POSITIONAL",
    "A mandatory positional argument",
  };
  optional_positional_argument<std::string> mOptionalPositional {};
  optional_positional_argument<std::vector<std::string>> mOptionalMulti {};
};

int main(int argc, char** argv) {
  const magic_args::extra_help extraHelp {
    .mDescription = "This program shows all the features.",
    .mVersion = "everything example v1.2.3",
    .mExamples = {
      "everything FOO",
      "everything --flag FOO",
      "everything --string someval FOO",
      "everything --string=someval FOO",
    },
  };
  const auto args = magic_args::parse<MyArgs>(argc, argv, extraHelp);
  if (!args.has_value()) {
    switch (args.error()) {
      case HelpRequested:
      case VersionRequested:
        return EXIT_SUCCESS;
      default:
        return EXIT_FAILURE;
    }
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}