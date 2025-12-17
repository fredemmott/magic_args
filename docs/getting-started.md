---
layout: default
title: Getting started
nav_order: 2
---

# Getting started
{: .no_toc}

## Table of contents
{: .no_toc }

- TOC
{:toc}

## Adding the library to your project's build system

There are 4 main ways to use *magic_args* that you can choose from:

- [vcpkg (recommended)](#vcpkg)
- [cmake](#cmake)
- [Manually adding *magic_args* to your include path](#manually-adding-magic_args-to-your-include-path)
- The [single-header version](#single-header-version)

### vcpkg

*recommended*

**magic_args** is available in the main [vcpkg](https://vcpkg.io) registry as `magic-args`.

While the main vcpkg registry is recommended for most users, newer or prerelease versions may
be available from [my vcpkg registry](https://github.com/fredemmott/vcpkg-registry);
see [Microsoft's instructions](https://learn.microsoft.com/en-us/vcpkg/consume/git-registries) on using third-party
registries.

### CMake

*magic_args* can be used in other cmake projects via `FetchContent`, `ExternalProject_Add()`, or `add_subdirectory()`.

For `add_subdirectory()`, any method can be used to pull in the files - e.g. git submodules, or just copying the files.

Once you've added `magic_args` to CMake, you can use the `magic_args` target.

### Manually adding *magic_args* to your include path

Add the `include/` subfolder of the *magic_args* source to your include path, then
`#include <magic_args/magic_args.hpp>`; no build step is
required.

You can fetch the source folder with any usual method, e.g. git submodules, or just copying the files.

### Single-header version

A single-header-file version is
available [from the releases page](https://github.com/fredemmott/magic_args/releases/latest). You can copy this single
file directly into your project, and `#include` it without additional dependencies

## `using namespace`

If you want to use `using namespace`, use `using namespace magic_args::public_api`; this
avoids pulling in the `magic_args::detail` namespace.

`magic_args` is designed to work easily regardless of whether you choose to use `using namespace` or not.

## Configuration

No configuration is usually required; however, you can choose to define the following macros:

### `MAGIC_ARGS_DISABLE_ENUM`

By default, *magic_args* will support enums if [`<magic_enum/magic_enum.hpp>` is available](https://github.com/Neargye/magic_enum). You can disable this behavior by defining this macro, which affects both the single-header and regular versions of *magic_args*.

You might want to define this macro to prevent *magic_args* from including `<magic_enum/magic_enum.hpp>`.

### `MAGIC_ARGS_ENABLE_SUBCOMMANDS`

Define this macro to enable support for subcommands in the single-header version of *magic_args*.

In the regular version, this has no effect; use `#include <magic_args/subcommands.hpp>` instead.

If you want to support both the single-header and regular version of *magic_args*:

```c++
#define MAGIC_ARGS_ENABLE_SUBCOMMANDS
#include <magic_args/magic_args.hpp>
#if __has_include(<magic_args/subcommands.hpp>)
#include <magic_args/subcommands.hpp>
#endif
```

### `MAGIC_ARGS_ENABLE_ICONV_EXTENSIONS`

*magic_args* can use `iconv` to convert argv to UTF-8 on Linux and macOS. This functionality is not enabled by default
as it requires linking with `iconv` on macOS; on Linux, iconv is usually part of libc.

If you are using the single-header version of *magic_args*, you can enable these helpers by defining this macro; otherwise, this macro has no effect; use `#include <magic_args/iconv.hpp>` instead.

### `MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS`

*magic_args* contains helpers for working with Win32 `WinMain()` and `wWinMain()` programs. They are not included by
default as they `#include <Windows.h>`.

If you are using the single-header version of *magic_args*, you can enable these helpers by defining this macro; otherwise, this macro has no effect; use `#include <magic_args/windows.hpp>` instead.

If you want to support both the single-header and regular version of *magic_args*:

```c++
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#include <magic_args/magic_args.hpp>
#if __has_include(<magic_args/windows.hpp>)
#include <magic_args/windows.hpp>
#endif
```

### Testing the configuration

*magic_args* makes the following available for your code:

- `MAGIC_ARGS_HAVE_ENUM`: defined if enum support has been included in the current file
- `MAGIC_ARGS_HAVE_SUBCOMMANDS`: defined if subcommand support has been included in the current file
- `MAGIC_ARGS_HAVE_ICONV_EXTENSIONS`: defined if the iconv extensions have been included in the current file
- `MAGIC_ARGS_HAVE_WINDOWS_EXTENSIONS`: defined if the windows extensions have been included in the current file
- `MAGIC_ARGS_CAN_CONVERT_TO_UTF8`: defined by either the iconv or windows extensions. If undefined, non-utf8 inputs may be an error
- `MAGIC_ARGS_SINGLE_FILE`: defined if the single-header-file version of *magic_args* has been included
- `magic_args::is_single_header_file`: `constexpr bool`, set to `true` if using the single-header-file version of *magic_args*, `false` otherwise