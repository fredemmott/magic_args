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

namespace magic_args::detail {
inline std::expected<void, incomplete_parse_reason> utf8_from_wide(
  std::string& buffer,
  const std::wstring_view wide) {
  if (wide.empty()) {
    buffer = {};
    return {};
  }
  const auto byteCount = WideCharToMultiByte(
    CP_UTF8,
    WC_ERR_INVALID_CHARS,
    wide.data(),
    static_cast<INT>(wide.size()),
    nullptr,
    0,
    nullptr,
    nullptr);
  if (byteCount <= 0) {
    return std::unexpected {invalid_encoding {}};
  }
  buffer.resize(byteCount);
  WideCharToMultiByte(
    CP_UTF8,
    WC_ERR_INVALID_CHARS,
    wide.data(),
    static_cast<INT>(wide.size()),
    buffer.data(),
    static_cast<INT>(buffer.size()),
    nullptr,
    nullptr);
  return {};
}

struct local_free_deleter {
  void operator()(auto ptr) const noexcept {
    LocalFree(ptr);
  }
};
}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <class T, class Traits = gnu_style_parsing_traits>
std::expected<T, incomplete_parse_reason> parse(
  const wchar_t* const commandLine,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  int argc {};
  std::unique_ptr<LPWSTR, detail::local_free_deleter> wargv {
    CommandLineToArgvW(commandLine, &argc)};

  std::vector<std::string> argv {};
  argv.reserve(argc);

  std::string buffer;
  buffer.resize(512);
  for (auto&& it: std::span {wargv.get(), static_cast<std::size_t>(argc)}) {
    if (const auto encode = detail::utf8_from_wide(buffer, it); !encode) {
      return std::unexpected {encode.error()};
    }
    argv.push_back(buffer);
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
    return std::unexpected {invalid_encoding {}};
  }
  std::wstring buffer;
  buffer.resize(charCount);
  MultiByteToWideChar(
    CP_ACP, MB_ERR_INVALID_CHARS, commandLine, -1, buffer.data(), charCount);
  return parse<T, Traits>(buffer.c_str(), help, outputStream, errorStream);
}

inline void attach_to_parent_terminal() {
  AttachConsole(ATTACH_PARENT_PROCESS);
  FILE* tmp {nullptr};
  struct Stream {
    DWORD mStdHandle;
    FILE* file;
    std::string name;
    std::string mode;
  };
  const Stream streams[] = {
    {STD_INPUT_HANDLE, stdin, "CONIN$", "r"},
    {STD_OUTPUT_HANDLE, stdout, "CONOUT$", "w"},
    {STD_ERROR_HANDLE, stderr, "CONOUT$", "w"},
  };
  for (auto&& [stdHandle, file, name, mode]: streams) {
    const HANDLE handle = GetStdHandle(stdHandle);
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
      freopen_s(&tmp, name.c_str(), mode.c_str(), file);
      tmp = {};
    }
  }
}

}// namespace magic_args::inline public_api
#endif