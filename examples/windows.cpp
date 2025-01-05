// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

/* If you're not using the single-header version, you can
 * include <magic_args/windows.hpp> instead of defining
 * MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS.
 *
 * This is off by default as depends on <Windows.h>
 */
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#include <magic_args/magic_args.hpp>
#include <thread>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  std::string mBaz;
};

int WINAPI wWinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPWSTR lpCmdLine,
  int nCmdShow) {
  magic_args::attach_to_parent_terminal();
  const auto args = magic_args::parse<MyArgs>(lpCmdLine);
  if (!args.has_value()) {
    if (args.error() == magic_args::HelpRequested) {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}