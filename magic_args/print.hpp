// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#if __has_include(<print>)
#include <print>
namespace magic_args {
template <class... Args>
void print(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
  std::print(file, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void println(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
  std::println(file, fmt, std::forward<Args>(args)...);
}
}// namespace magic_args
#else
#include <format>

namespace magic_args {
template <class... Args>
void print(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
  const auto buffer = std::format(fmt, std::forward<Args>(args)...);
  std::fwrite(buffer.data(), 1, buffer.size(), file);
}

template <class... Args>
void println(FILE* file, std::format_string<Args...> fmt, Args&&... args) {
  const auto buffer = std::format(fmt, std::forward<Args>(args)...) + "\n";
  std::fwrite(buffer.data(), 1, buffer.size(), file);
}
}// namespace magic_args
#endif
