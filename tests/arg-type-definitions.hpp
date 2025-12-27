// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

struct EmptyStruct {};

namespace MyNS {
struct MyValueType {
  static constexpr std::string_view InvalidValue {"___MAGIC_INVALID___"};
  std::string mValue;
  constexpr bool operator==(const MyValueType&) const noexcept = default;
};
std::expected<void, magic_args::invalid_argument_value> from_string_argument(
  MyValueType& v,
  std::string_view arg) {
  if (arg == MyValueType::InvalidValue) {
    return std::unexpected {magic_args::invalid_argument_value {}};
  }
  v.mValue = std::string {arg};
  return {};
}
}// namespace MyNS
using MyNS::MyValueType;

template <magic_args::parsing_traits T>
struct BasicCustomArgs {
  using parsing_traits = T;
  MyValueType mRaw;
  magic_args::option<MyValueType> mOption {
    .mHelp = "std::optional",
  };
  magic_args::optional_positional_argument<MyValueType> mPositional;

  constexpr bool operator==(const BasicCustomArgs&) const noexcept = default;
};
using CustomArgs = BasicCustomArgs<magic_args::gnu_style_parsing_traits>;
using CustomArgsPS
  = BasicCustomArgs<magic_args::powershell_style_parsing_traits>;

template <magic_args::parsing_traits T>
struct BasicNormalization {
  using parsing_traits = T;

  std::string mEmUpperCamel;
  std::string m_EmUnderscoreUpperCamel;
  std::string _UnderscoreUpperCamel;
  std::string _underscoreLowerCamel;
  std::string UpperCamel;
  std::string lowerCamel;
  std::string m_em_snake_case;
  std::string snake_case;
  std::string member_starts_with_m_but_is_not_m_prefix;
};
using Normalization = BasicNormalization<magic_args::gnu_style_parsing_traits>;
using NormalizationPS
  = BasicNormalization<magic_args::powershell_style_parsing_traits>;

struct Optional {
  std::optional<std::string> mValue;
  magic_args::option<std::optional<std::string>> mDocumentedValue {
    .mHelp = "documented value",
  };
  magic_args::optional_positional_argument<std::optional<std::string>>
    mPositional {
      .mHelp = "absent != empty",
    };
};

struct FlagsOnly {
  bool mFoo {false};
  bool mBar {false};
  magic_args::flag mBaz {
    false,
    "baz",
    "do the bazzy thing",
    "b",
  };

  bool operator==(const FlagsOnly&) const noexcept = default;
};

struct ShortFlags {
  magic_args::flag mFlagA {.mShortName = "a"};
  magic_args::flag mFlagB {.mShortName = "b"};
  magic_args::flag mFlagC {.mShortName = "c"};
};

struct OptionsOnly {
  std::string mString;
  int mInt {0};
  magic_args::option<std::string> mDocumentedString {
    {},
    "foo",
    "do the foo thing",
    "f",
  };

  bool operator==(const OptionsOnly&) const noexcept = default;
};

struct FlagsAndPositionalArguments {
  bool mFlag {false};
  magic_args::optional_positional_argument<std::string> mInput;
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
};

struct MandatoryPositionalArgument {
  bool mFlag {false};
  magic_args::mandatory_positional_argument<std::string> mInput;
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
};

struct MultiValuePositionalArgument {
  bool mFlag {false};
  magic_args::optional_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
  magic_args::optional_positional_argument<std::vector<std::string>> mInputs;
};

struct MandatoryMultiValuePositionalArgument {
  bool mFlag {false};
  magic_args::mandatory_positional_argument<std::string> mOutput {
    .mHelp = "file to create",
  };
  magic_args::mandatory_positional_argument<std::vector<std::string>> mInputs;
};

struct CustomPositionalArgument {
  magic_args::optional_positional_argument<MyValueType> mFoo;
};
