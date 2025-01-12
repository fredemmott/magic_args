// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

namespace magic_args::inline public_api {

enum class incomplete_parse_reason {
  HelpRequested,
  VersionRequested,
  MissingRequiredArgument,
  MissingArgumentValue,
  InvalidArgument,
  InvalidArgumentValue,
  InvalidEncoding,
};
using enum incomplete_parse_reason;

}// namespace magic_args::inline public_api