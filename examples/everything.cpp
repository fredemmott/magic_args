// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

using namespace magic_args::public_api;

struct MyCustomType {
  std::string mValue;
};
std::expected<void, invalid_argument_value> from_string_argument(
  MyCustomType& value,
  std::string_view arg) {
  value.mValue = arg;
  return {};
}
std::string formattable_argument_value(const MyCustomType& value) {
  return value.mValue;
}

struct MyArgs {
  static constexpr auto description = "This program shows all the features.";
  static constexpr auto version = "everything example v1.2.3";
  static constexpr auto examples = {
    "everything FOO",
    "everything --flag FOO",
    "everything --string someval FOO",
    "everything --string=someval FOO",
  };

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

#ifdef MAGIC_ARGS_ENUM_HPP
  enum class MyEnum {
    Foo,
    Bar,
  };

  MyEnum mEnum;
#endif
};

MAGIC_ARGS_MAIN(MyArgs&& args) {
  magic_args::dump(args);
  return EXIT_SUCCESS;
}