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

## Quick guide

- **macOS and UTF-8 Linux only?** ➡️ Use `main()` with `utf8 = std::span(argv, argc)` from `<magic_args/iconv.hpp>`
- **Windows CLI only?** ➡️ Use `wmain()` with `utf8 = make_utf8_argv(argc, argv)` from `<magic_args/windows.hpp>`
- **Maximum portability?** ➡️ Use `wmain()` on Windows, `main()` on other platforms; for both, use `utf8 = make_utf8_argv(argc, argv)` from the corresponding header

For other cases, see the details below.

## Which encodings are considered "UTF-8"?

Under Windows:
- UTF-8 (codepage 65001)
- 7-bit US-ASCII (codepage 20127)

On Unix-like systems (including Linux and macOS):
- `"UTF-8"`
- `"US-ASCII"` (7-bit)
- `"ANSI_X3.4-1968"`

7-bit US-ASCII strings are treated as UTF-8 because they have the same byte sequence when converted to UTF-8.

Technically, `"ANSI_X3.4-1968"` means 7-bit US-ASCII; in practice, it usually represents the `"C"` locale, which is almost always either UTF-8 or 7-bit US-ASCII, so we can treat it as UTF-8.

When *magic_args* detects a UTF-8-compatible input encoding, it does not *convert* to UTF-8, but it does *validate* that the input is UTF-8:
- on Windows, conversion to-and-from UTF-16 is often necessary, which will fail on invalid input. *magic_args* also validates on other platforms so that application behavior is consistent across platforms
- the performance cost is small

## If you want to assume UTF-8 on all platforms

The main entrypoints (e.g. `parse()`) take a UTF-8 range as input. This must be indexable, and the elements must be convertible to `std::string_view`. *magic_args* will *assume* this range is UTF-8.

If you are happy to assume UTF-8, you can use a standard C++ range, e.g.:

- `magic_args::parse<MyArgs>(std::views::counted(argv, argc)`
- `magic_args::parse<MyArgs>(std::span(argv, argc))`

## If you're happy to assume UTF-8 on Linux and macOS but want to handle Windows

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

#include <print>

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

- The default implementation on Linux and macOS: this will return an error if the locale (as defined by the environment variables and system settings) is not compatible with UTF-8
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

#include <print>

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

## Avoiding `wmain()`

`int wmain(int argc, wchar_t** argv)` is a non-standard Microsoft extension; if you'd like to avoid using Microsoft extensions:

```c++
int main(
  [[maybe_unused]] int argc,
  [[maybe_unused]] char** argv) {
#ifdef _WIN32
    /** Uses `GetCommandLineW()`, `CommandLineToArgvW()`, then converts to UTF-8.
     *
     * The MS CRT will still make the potentially-lossy conversion from UTF-16 to the active code page and pass the
     * result to `main()`, but as the implementation calls `GetCommandLineW()`, we're not using that lossy data
     */
    const auto utf8 = magic_args::make_utf8_argv();
#else
    const auto utf8 = magic_args::make_utf8_argv(argc, argv);
#endif
    // ... Remainder of the code is the same as previous examples
}
```

## `WinMain(...)` and `wWinMain(...)`

Use `#include <magic_args/windows.hpp>` and `magic_args::make_utf8_argv()`, without passing any arguments to `make_utf8_argv()`.

{: .warning }
DO NOT pass `lpCmdLine` to `make_utf8_argv()`; Windows does not consistently include the program name/path in this
parameter - the traditional `argv[0]`. If you omit the parameter, *magic_args* will call `GetCommandLineW()` and
`CommandLineToArgvW()` internally, which will consistently include `argv[0]`, while also avoid encoding issues. While
passing an `lpCmdLine` to `make_utf8_argv()` is supported, this is primarily intended for automated tests.

Avoid `WinMain` and `wWinMain` unless you *need* a single executable to be launchable both as a CLI and a GUI
application. I *strongly* recommend avoiding this if at all possible: if an executable uses `WinMain` or `wWinMain`, CLI
operations will usually create a new console window, even if the application was launched from a terminal.

*magic_args* contains `magic_args::win32::attach_to_parent_terminal()` helper for this case, but it is 'best-effort':
Windows is not designed to support mixed-mode programs like these, and there are issues, such as shell prompts
being written over/underneath command output, and misbehaving control characters. Behavior also varies between terminal
programs, e.g. between the modern 'Terminal' app and the classic `cmd.exe` host process.

## Compilation and link options

- Under MSVC and clang-cl, using `/utf-8` is recommended, but not required. clang and g++ will use UTF-8 by default
- If you've chosen to use iconv:
  - on macOS, you must explicitly link against the iconv library
  - on Linux, iconv is usually part of libc, so no additional flags are needed
  - while there are ports of iconv to Windows, *magic_args* is not tested with these; `<magic_args/windows.hpp>` is strongly encouraged instead

If you are using CMake:

```cmake
target_compile_options(my_target PRIVATE "$<$<CXX_COMPILER_ID:MSVC>/utf-8>")
if (APPLE)
  find_package(Iconv REQUIRED)
  target_link_libraries(my_target PRIVATE Iconv::Iconv)
endif ()
```

## Process vs environment locale on Unix-like systems

On Linux and macOS, *magic_args* ignores the *process* locale, and instead looks at the *environment* locale via `newlocale(3)` and `nl_langinfo_l(3)`.  This is because argv is provided by the caller, so the caller is ultimately responsible for passing argv in a way that matches `LC_CTYPE`.

*magic_args* will not set the process locale; this was considered potentially surprising, and out of scope. If you wish to use the environment locale as the process locale, call `setlocale(LC_ALL, "")` or similar as usual.

If you want different behavior, you can:
- convert `argv` to UTF-8 yourself, and pass the range to `magic_args::parse()` without calling `magic_args::make_utf8_argv()`
- include `<magic_args/iconv.hpp>` and use `magic_args::make_utf8_argv(argc, argv, charset)`, where `charset` is a string recognized by iconv, e.g. `"ISO-8859-2"`

For a given locale, you can retrieve the encoding with `LANG=xx_YY locale charmap` - while the charset is defined by `LC_CTYPE`, the strings are different. For example:

```
$ LC_CTYPE=pl_PL locale charmap
ISO-8859-2
```

[app-manifest-utf8]: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page