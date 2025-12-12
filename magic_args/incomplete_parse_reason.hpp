// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <variant>

namespace magic_args::inline public_api {
inline namespace incomplete_parse_reasons {

struct help_requested {};
struct version_requested {};
struct missing_required_argument {};
struct missing_argument_value {};
struct invalid_argument {};
struct invalid_argument_value {};
struct invalid_encoding {};

}// namespace incomplete_parse_reasons

using incomplete_parse_reason = std::variant<
  help_requested,
  version_requested,
  missing_required_argument,
  missing_argument_value,
  invalid_argument,
  invalid_argument_value,
  invalid_encoding>;

}// namespace magic_args::inline public_api