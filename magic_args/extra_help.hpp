// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>

namespace magic_args::inline public_api {
struct extra_help {
  std::string mDescription;
  std::string mVersion;
  std::vector<std::string> mExamples;
};
}// namespace magic_args::inline public_api