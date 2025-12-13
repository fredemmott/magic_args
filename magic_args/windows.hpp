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

namespace magic_args::detail::win32 {
inline std::expected<void, incomplete_parse_reason_t> utf8_from_wide(
  std::string& buffer,
  const std::wstring_view wide) {
  if (wide.empty()) {
    buffer = {};
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
  if (byteCount <= 0) {
    return std::unexpected {invalid_encoding {}};
  }

  buffer.resize_and_overwrite(byteCount, convert);
  if (buffer.empty()) {
    return std::unexpected {invalid_encoding {}};
  }
  return {};
}

struct local_free_deleter {
  void operator()(auto ptr) const noexcept {
    LocalFree(ptr);
  }
};

inline std::expected<std::vector<std::string>, incomplete_parse_reason_t>
make_argv(const wchar_t* commandLine) {
  int argc {};
  const std::unique_ptr<LPWSTR, local_free_deleter> wargv {
    CommandLineToArgvW(commandLine, &argc)};
  std::vector<std::string> argv;
  argv.reserve(argc);

  std::string buffer;
  for (auto&& arg: std::span {wargv.get(), static_cast<std::size_t>(argc)}) {
    if (const auto converted = utf8_from_wide(buffer, arg); !converted) {
      return std::unexpected {converted.error()};
    }
    argv.push_back(buffer);
  }

  return argv;
}

inline std::expected<std::vector<std::string>, incomplete_parse_reason_t>
make_argv(const std::string_view commandLine) {
  const auto convert
    = [&commandLine](wchar_t* out, const std::size_t wideCount) {
        return MultiByteToWideChar(
          CP_ACP,
          MB_ERR_INVALID_CHARS,
          commandLine.data(),
          static_cast<INT>(commandLine.size()),
          out,
          static_cast<INT>(wideCount));
      };
  const auto wideCount = convert(nullptr, 0);
  std::wstring buffer;
  buffer.resize_and_overwrite(wideCount, convert);
  if (buffer.empty()) {
    return std::unexpected {invalid_encoding {}};
  }
  return make_argv(buffer.c_str());
}

}// namespace magic_args::detail::win32

namespace magic_args::inline public_api {
template <class T, class Traits = gnu_style_parsing_traits, class TChar>
std::expected<T, incomplete_parse_reason_t> parse(
  const TChar* const commandLine,
  const program_info& help = {},
  FILE* outputStream = stdout,
  FILE* errorStream = stderr) {
  const auto argv = detail::win32::make_argv(commandLine);
  if (!argv) {
    return std::unexpected {argv.error()};
  }
  return parse<T, Traits>(*argv, help, outputStream, errorStream);
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
