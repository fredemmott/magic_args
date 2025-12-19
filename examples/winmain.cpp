// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

/* If you're not using the single-header version, you can
 * include <magic_args/windows.hpp> instead of defining
 * MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS.
 *
 * This is off by default as it depends on <Windows.h>
 */
#include <magic_args/magic_args.hpp>
#include <magic_args/windows.hpp>
#include <thread>

#include <Windows.h>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  std::string mBaz;
};

int WINAPI wWinMain(
  [[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] LPWSTR lpCmdLine,
  [[maybe_unused]] int nCmdShow) {
  magic_args::win32::attach_to_parent_terminal();

  // *NOT* passing in `lpCmdLine`, because Windows is inconsistent in whether
  // that includes the program name. If a command line is not provided,
  // magic_args will use `GetCommandLineW()`.
  const auto argv = magic_args::make_utf8_argv();
  if (!argv) {
    std::println(stderr, "Couldn't convert argv to utf8");
    return EXIT_FAILURE;
  }

  const auto args = magic_args::parse<MyArgs>(*argv);
  if (!args.has_value()) {
    // This could be an actual error, e.g. invalid argument,
    // or something like `--help` or `--version`, which while not an error,
    // are an 'unexpected' outcome in the std::expected
    return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}