// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#if (!defined(MAGIC_ARGS_SINGLE_FILE)) \
  || defined(MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS) \
    && !defined(MAGIC_ARGS_WINDOWS_HPP)
#define MAGIC_ARGS_WINDOWS_HPP

#define MAGIC_ARGS_HAVE_WINDOWS_EXTENSIONS

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

inline std::expected<void, DWORD> wide_from_codepage(
  std::wstring& buffer,
  const std::string_view data,
  const UINT codePage) {
  if (data.empty()) {
    buffer.clear();
    return {};
  }

  const auto convert
    = [&data, codePage](wchar_t* out, const std::size_t wideCount) {
        return MultiByteToWideChar(
          codePage,
          MB_ERR_INVALID_CHARS,
          data.data(),
          static_cast<INT>(data.size()),
          out,
          static_cast<INT>(wideCount));
      };
  const auto wideCount = convert(nullptr, 0);
  if (wideCount == 0) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }
  buffer.resize_and_overwrite(wideCount, convert);
  if (buffer.empty()) [[unlikely]] {
    return std::unexpected {GetLastError()};
  }

  return {};
}

inline std::expected<void, DWORD> utf8_from_codepage(
  std::string& utf8,
  const std::string_view data,
  const UINT codePage) {
  if (data.empty()) {
    utf8.clear();
    return {};
  }
  std::wstring wide;
  if (const auto widen = wide_from_codepage(wide, data, codePage); !widen)
    [[unlikely]] {
    return std::unexpected {widen.error()};
  }
  if (const auto converted = utf8_from_wide(utf8, wide); !converted)
    [[unlikely]] {
    return std::unexpected {converted.error()};
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
  std::wstring buffer;
  if (const auto widen
      = detail::win32::wide_from_codepage(buffer, commandLine, codePage);
      !widen) {
    return std::unexpected {widen.error()};
  }
  return make_argv(buffer.c_str());
}

inline std::expected<std::vector<std::string>, DWORD> make_argv(
  const int argc,
  const char* const* argv,
  const UINT codePage = CP_ACP) {
  if (argc == 0 || !argv) [[unlikely]] {
    return std::unexpected {ERROR_INVALID_PARAMETER};
  }
  if (codePage == CP_UTF8 || (codePage == CP_ACP && GetACP() == CP_UTF8)) {
    // Even though we don't need to convert it, validate it.
    //
    // This is to ensure consistency with `make_argv("foo bar");`
    for (auto&& arg: std::views::counted(argv, argc)) {
      if (!arg) [[unlikely]] {
        return std::unexpected {ERROR_INVALID_PARAMETER};
      }
      const std::string_view argView {arg};
      if (argView.empty()) {
        // Empty args are valid
        continue;
      }
      const auto wideCount = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        argView.data(),
        static_cast<int>(argView.size()),
        nullptr,
        0);
      if (wideCount == 0) [[unlikely]] {
        return std::unexpected {GetLastError()};
      }
    }
    return std::views::counted(argv, argc)
      | std::ranges::to<std::vector<std::string>>();
  }

  std::vector<std::string> ret;
  ret.reserve(argc);

  std::string buffer;
  buffer.reserve(1024);

  for (auto&& arg: std::views::counted(argv, argc)) {
    const auto converted
      = detail::win32::utf8_from_codepage(buffer, arg, codePage);
    if (!converted) [[unlikely]] {
      return std::unexpected {converted.error()};
    }
    ret.push_back(buffer);
  }
  return ret;
}

}// namespace magic_args::inline public_api::win32

#endif
