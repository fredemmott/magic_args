// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

namespace magic_args::inline api {

enum class incomplete_parse_reason {
  HelpRequested,
  VersionRequested,
  MissingRequiredArgument,
  MissingArgumentValue,
  InvalidArgument,
  InvalidArgumentValue,
};

}