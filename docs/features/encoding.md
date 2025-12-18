---
layout: default
title: Encodings
---

# Encodings
{: .no_toc }

*magic_args* is built for [UTF-8 Everywhere](https://utf8everywhere.org/); while on Linux and macOS, you can largely assume UTF-8, there are steps you can take to improve the reliability and portability of your program.

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

## If you want to assume UTF-8 on all platforms

The main entrypoints (e.g. `parse()`) take a UTF-8 range as input. This must be indexable, and the elements must be convertible to `std::string_view`. *magic_args* will *assume* this range is UTF-8.

If you are happy to assume UTF-8, you can use a standard C++ range, e.g.:

- `magic_args::parse<MyArgs>(std::views::counted(argv, argc)`
- `magic_args::parse<MyArgs>(std::span(argv, argc))`

## If you're happy to assume UTF-8 on Linux and macOS, but want to handle Windows

Windows is particularly problematic for CLI encodings:

- UTF-8 user locales are extremely rare and often cause program-level issues
- Command line arguments on Windows are natively UTF-16, not just bytes. For classic `int main(int argc, char** argv)` programs, Windows will do a *potentially lossy* conversion from UTF-16 to the user's active code page.

You have two main options:

- on Windows 1903 and above, you can specify that your *process* always launches with UTF-8 as the active code page, regardless of the user's active code page, [via the application manifest][app-manifest-utf8]
- implement `int wmain(int argc, wchar_t** argv)` instead of main

To opt your process into UTF-8, see [Microsoft's documentation][app-manifest-utf8].

To use `wmain()` in a Windows-only-CLI:

```c++
#include <magic_args/magic_args.hpp>
#include <magic_args/windows.hpp>

#Include <print>

struct MyArgs {
    std::string foo;
};

int wmain(int argc, wchar_t** argv) {
  const auto utf8 = magic_args::make_utf8_argv(argc, argv);
  if (!utf8) {
    std::println("Couldn't convert argv to UTF-8");
    return EXIT_FAILURE;
  }

  const auto args = magic_args::parse<MyArgs>(*utf8);
  if (args) {
    // your code here
    magic_args::dump(*args);
    return EXIT_SUCCESS;
  }
  return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
}
```

## Portable and reliable handling

*magic_args* includes 3 implementations of `make_utf8_argv(argc, argv)`:

- The default implementation on Linux and macOS: this will raise an error if the locale (as defined by the environment variables and system settings) is UTF-8
- `<magic_args/windows.hpp>`: this converts encodings using Windows-only functions, via `<Windows.h>`
- `<magic_args/iconv.hpp>`: this uses libiconv (generally part of libc on Linux) to convert between encodings. If you use this header on macOS, you will need to also link against the iconv library.

Neither `windows.hpp` nor `iconv.hpp` are included automatically, because they add dependencies. To make the most portable code, you need to use both, and `wmain()`:

```c++
#include <magic_args/magic_args.hpp>
#ifdef _WIN32
#include <magic_args/windows.hpp>
#else
#include <magic_args/iconv.hpp>
#endif

#Include <print>

struct MyArgs {
  std::string foo;
};

static int generic_main(const auto argc, const auto* const* argv) {
  const auto utf8 = magic_args::make_utf8_argv(argc, argv);
  if (!utf8) {
    std::println("Couldn't convert argv to UTF-8");
    return EXIT_FAILURE;
  }

  const auto args = magic_args::parse<MyArgs>(*utf8);
  if (args) {
    // your code here
    magic_args::dump(*args);
    return EXIT_SUCCESS;
  }
  return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
  return generic_main(argc, argv);
}
#else
int main(int argc, char** argv) {
  return generic_main(argc, argv);
}
#endif
```

While this code could also work with `main()` on Windows, using `wmain()` has advantages:

- When the process code page is *not* UTF-8, it avoids an extra conversion to the active code page
- It gets you Unicode support back to Windows 2000/XP/Vista; the process code page option is only functional on Windows 10 1903 and later

[app-manifest-utf8]: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page