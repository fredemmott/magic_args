// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_PROGRAM_INFO_HPP
#define MAGIC_ARGS_PROGRAM_INFO_HPP
#include <string>
#include <vector>

namespace magic_args::inline public_api {
struct program_info {
  std::string mDescription;
  std::string mVersion;
  std::vector<std::string> mExamples;
};
}// namespace magic_args::inline public_api
#endif