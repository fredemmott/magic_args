// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <string_view>

constexpr std::string_view chomp(
  std::string_view sv,
  const std::size_t count = 1) {
  sv.remove_prefix(count);
  return sv;
}
template <std::size_t N>
constexpr std::string_view chomp(
  const char (&literal)[N],
  const std::size_t count = 1) {
  return chomp(std::string_view {literal, N - 1}, count);
}

static_assert(chomp("foo") == std::string_view {"oo"});
