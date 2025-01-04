// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>

namespace magic_args::inline public_api {
struct extra_help {
  std::string mDescription;
  std::vector<std::string> mExamples;
  std::string mVersion;
};
}// namespace magic_args::inline api