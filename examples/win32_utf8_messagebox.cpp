// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include "win32_utf8_messagebox.hpp"

#include <string>

// For this sample, we want *a* way to show text without a terminal
void utf8_messagebox(const std::string_view utf8, const unsigned int flags) {
  const auto convert = [&](wchar_t* buffer, const std::size_t bufferSize) {
    return static_cast<std::size_t>(MultiByteToWideChar(
      CP_UTF8,
      MB_ERR_INVALID_CHARS,
      utf8.data(),
      static_cast<DWORD>(utf8.size()),
      buffer,
      static_cast<DWORD>(bufferSize)));
  };
  const auto charCount = convert(nullptr, 0);
  std::wstring wide;
  wide.resize_and_overwrite(charCount, convert);
  MessageBoxW(nullptr, wide.c_str(), L"magic_args sample", MB_OK | flags);
}
