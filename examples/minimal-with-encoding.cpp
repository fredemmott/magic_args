// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#ifdef _WIN32
#include <magic_args/windows.hpp>
#endif

struct MyArgs {
  bool foo {false};
  std::string bar;
  int baz {0};
};

static int generic_main(const auto argc, const auto* const* argv) {
  const auto utf8 = magic_args::make_utf8_argv(argc, argv);
  if (!utf8) {
    std::println("Couldn't convert argv to UTF-8");
    return EXIT_FAILURE;
  }

  const auto args = magic_args::parse<MyArgs>(*utf8);
  if (args) {
    magic_args::dump(*args);
    return EXIT_SUCCESS;
  }
  return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  return EXIT_SUCCESS;
}

#ifdef _WIN32
// Get UTF-16 argv regardless of the active code page
int wmain(int argc, wchar_t** argv) {
  return generic_main(argc, argv);
}
#else
/* Standard main.
 *
 * You can also use this on Windows, but the `wwmain()` is generally more
 * predictable.
 *
 * As of Windows 1903, you can force this to be UTF-8 via your application
 * manifest:
 *
 * https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
 */
int main(int argc, char** argv) {
  return generic_main(argc, argv);
}
#endif