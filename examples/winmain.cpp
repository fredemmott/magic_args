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

struct MyApp {
  struct arguments_type {
    bool mFoo {false};
    std::string mBar;
    std::string mBaz;
  };

  static int
  main(const arguments_type& args, const HINSTANCE, const int /*nCmdShow*/) {
    magic_args::capturing_console_output output;
    magic_args::dump(args, output.out);

    utf8_messagebox(std::move(output).out_str(), MB_ICONINFORMATION);
    return EXIT_SUCCESS;
  }

  static int unparsed_arguments_main(
    const magic_args::incomplete_parse_reason_t& reason,
    const std::string& output,
    const HINSTANCE,
    const int /*nCmdShow*/) {
    utf8_messagebox(
      output, is_error(reason) ? MB_ICONERROR : MB_ICONINFORMATION);
    return is_error(reason) ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  static int argv_encoding_error_main(
    const magic_args::make_utf8_argv_error_t&,
    const std::string& output,
    const HINSTANCE,
    const int /* nCmdShow */) {
    utf8_messagebox(output, MB_ICONERROR);
    return EXIT_FAILURE;
  }
};

MAGIC_ARGS_WINMAIN(MyApp)