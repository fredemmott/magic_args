// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#if (!defined(MAGIC_ARGS_SINGLE_FILE)) \
  || defined(MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS)
#define MAGIC_ARGS_WINDOWS_HPP

#include <Windows.h>

#include <string>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "parse.hpp"
#endif

namespace magic_args::detail::win32 {
inline std::expected<void, DWORD> utf8_from_wide(
  std::string& buffer,
  const std::wstring_view wide) {
  if (wide.empty()) {
    buffer.clear();
    return {};
  }
  const auto convert = [&wide](const LPSTR out, const std::size_t byteCount) {
    return WideCharToMultiByte(
      CP_UTF8,
      WC_ERR_INVALID_CHARS,
      wide.data(),
      static_cast<INT>(wide.size()),
      out,
      static_cast<INT>(byteCount),
      nullptr,
      nullptr);
  };
  const auto byteCount = convert(nullptr, 0);
  if (byteCount <= 0) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }

  buffer.resize_and_overwrite(byteCount, convert);
  if (buffer.empty()) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }
  return {};
}

struct local_free_deleter {
  void operator()(auto ptr) const noexcept {
    LocalFree(ptr);
  }
};

}// namespace magic_args::detail::win32

namespace magic_args::inline public_api::win32 {

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

/** Returns a UTF-8 encoded argv, or a Win32 error code.
 *
 * Returns `ERROR_INVALID_PARAMETER` on empty or nullptr input, or
 * `GetLastError()` for other issues.
 *
 * `ERROR_NO_UNICODE_TRANSLATION` (1113, 0x459) is a likely error code.
 */
inline std::expected<std::vector<std::string>, DWORD> make_argv(
  const wchar_t* commandLine = GetCommandLineW()) {
  if (commandLine == nullptr || commandLine[0] == 0) [[unlikely]] {
    return std::unexpected {ERROR_INVALID_PARAMETER};
  }
  using namespace detail::win32;
  int argc {};
  const std::unique_ptr<LPWSTR, local_free_deleter> wargv {
    CommandLineToArgvW(commandLine, &argc)};
  if (!wargv) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }
  std::vector<std::string> argv;
  argv.reserve(argc);

  std::string buffer;
  for (auto&& arg: std::span {wargv.get(), static_cast<std::size_t>(argc)}) {
    if (const auto converted = utf8_from_wide(buffer, arg); !converted)
      [[unlikely]] {
      return std::unexpected {converted.error()};
    }
    argv.push_back(buffer);
  }

  return argv;
}

/** Returns a UTF-8 encoded argv, or a Win32 error code.
 *
 * Returns `ERROR_INVALID_PARAMETER` on empty input, or
 * `GetLastError()` for other issues.
 *
 * `ERROR_NO_UNICODE_TRANSLATION` (1113, 0x459) is a likely error code.
 */
inline std::expected<std::vector<std::string>, DWORD> make_argv(
  const std::string_view commandLine,
  const UINT codePage = CP_ACP) {
  if (commandLine.empty()) [[unlikely]] {
    return std::unexpected {ERROR_INVALID_PARAMETER};
  }
  const auto convert
    = [&commandLine, codePage](wchar_t* out, const std::size_t wideCount) {
        return MultiByteToWideChar(
          codePage,
          MB_ERR_INVALID_CHARS,
          commandLine.data(),
          static_cast<INT>(commandLine.size()),
          out,
          static_cast<INT>(wideCount));
      };
  const auto wideCount = convert(nullptr, 0);
  if (wideCount == 0) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }
  std::wstring buffer;
  buffer.resize_and_overwrite(wideCount, convert);
  if (buffer.empty()) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }
  return make_argv(buffer.c_str());
}
}// namespace magic_args::inline public_api::win32

#endif
