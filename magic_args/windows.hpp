// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifdef __CLION_IDE__
#define MAGIC_ARGS_ENABLE_WINDOWS
#endif

#if (defined(MAGIC_ARGS_ENABLE_WINDOWS) || !defined(MAGIC_ARGS_SINGLE_FILE)) \
  && !defined(MAGIC_ARGS_WINDOWS_HPP)
#define MAGIC_ARGS_WINDOWS_HPP

#include <Windows.h>

#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "parse.hpp"
#endif

namespace magic_args::inline public_api {

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
  const wchar_t* const commandLine,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  int argc {};
  auto wargv = CommandLineToArgvW(commandLine, &argc);
  std::vector<std::string> argv;
  argv.reserve(argc);
  std::vector<char> buffer;
  for (auto&& it: std::span {wargv, static_cast<std::size_t>(argc)}) {
    const std::wstring_view warg {it};
    if (warg.empty()) {
      argv.emplace_back();
      continue;
    }
    const auto byteCount = WideCharToMultiByte(
      CP_UTF8,
      WC_ERR_INVALID_CHARS,
      warg.data(),
      warg.size(),
      nullptr,
      0,
      nullptr,
      nullptr);
    if (byteCount <= 0) {
      return std::unexpected {incomplete_parse_reason::InvalidEncoding};
    }
    buffer.clear();
    buffer.resize(byteCount);
    WideCharToMultiByte(
      CP_UTF8,
      WC_ERR_INVALID_CHARS,
      warg.data(),
      warg.size(),
      buffer.data(),
      buffer.size(),
      nullptr,
      nullptr);
    argv.emplace_back(buffer.data(), byteCount);
  }

  std::vector<std::string_view> argvsv {argv.begin(), argv.end()};
  return parse<T, Traits>(std::span {argvsv}, help, outputStream, errorStream);
}

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
  const char* const commandLine,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  // There's CommandLineToArgvW, but no CommandLineToArgvA
  const auto charCount = MultiByteToWideChar(
    CP_ACP, MB_ERR_INVALID_CHARS, commandLine, -1, nullptr, 0);
  if (charCount <= 0) {
    return std::unexpected {incomplete_parse_reason::InvalidEncoding};
  }
  std::wstring buffer;
  buffer.resize(charCount);
  MultiByteToWideChar(
    CP_ACP, MB_ERR_INVALID_CHARS, commandLine, -1, buffer.data(), charCount);
  return parse<T, Traits>(buffer.c_str(), help, outputStream, errorStream);
}

}// namespace magic_args::inline public_api
#endif