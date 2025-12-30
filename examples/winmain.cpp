// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <Windows.h>

// For this sample, we want *a* way to show text without a terminal
static void utf8_messagebox(const std::string& utf8, const unsigned int flags) {
  const auto convert = [&](wchar_t* buffer, const std::size_t bufferSize) {
    return static_cast<std::size_t>(MultiByteToWideChar(
      CP_UTF8,
      MB_ERR_INVALID_CHARS,
      utf8.c_str(),
      static_cast<DWORD>(utf8.size()),
      buffer,
      static_cast<DWORD>(bufferSize)));
  };
  const auto charCount = convert(nullptr, 0);
  std::wstring wide;
  wide.resize_and_overwrite(charCount, convert);
  MessageBoxW(nullptr, wide.c_str(), L"magic_args sample", MB_OK | flags);
}

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  std::string mBaz;
};

// Could also use `magic_args::winmain_expected_t<MyArgs>, but this is clearer
MAGIC_ARGS_WINMAIN(
  const std::expected<MyArgs, magic_args::winmain_unexpected_t>& args,
  HINSTANCE,
  const int /*nCmdShow*/) {
  if (args) [[likely]] {
    magic_args::capturing_console_output output;
    magic_args::dump(*args, output.out);
    utf8_messagebox(std::move(output).out_str(), MB_ICONINFORMATION);
    return EXIT_SUCCESS;
  }

  // `--version` and `--help` hit this path, but aren't errors
  const auto isError = magic_args::is_error(args.error());
  utf8_messagebox(
    args.error().output, isError ? MB_ICONERROR : MB_ICONINFORMATION);
  return isError ? EXIT_FAILURE : EXIT_SUCCESS;
}