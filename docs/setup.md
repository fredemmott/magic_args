---
layout: default
title: Setup
nav_order: 2
---

# Setup
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

## macOS

To use *magic_args* on macOS, you must choose between three options:

- *recommended*: link against `iconv`: the program will convert encodings as necessary. This option is recommended because it gives you consistent behavior on all major platforms
- define `MAGIC_ARGS_DISABLE_ICONV`: if the character set is not UTF-8 or a compatible 7-bit encoding (e.g. US-ASCII), an error will be raised. This is slightly simpler, and may be acceptable as non-UTF-8 locales are extremely rare on macOS
- handle (or ignore) encoding issues yourself, and call the *magic_args* functions directly instead of using the `MAGIC_ARGS_*_MAIN()` macros

If you are using CMake, you can follow the recommended path with the following code:

```cmake
if (APPLE)
  find_package(Iconv REQUIRED)
  target_link_libraries(my_executable PRIVATE Iconv::Iconv)
endif ()
```

This does not affect other platforms:

- on Windows, the Win32 API is used instead of `iconv`
- on Linux, `iconv` is typically provided by libc and doesn't require explicit linking

## Configuration

*magic_args* recognizes the following preprocessor definitions:

- `MAGIC_ARGS_DISABLE_ICONV`: disable support for using `iconv()` for converting argv to UTF-8 
  - this is automatically defined on Windows, as the Win32 API is used instead
  - [you *might* want to set this on macOS](#macos)
- `MAGIC_ARGS_DISABLE_ENUM`: disables support for enums
  - this is automatically defined if the `<magic_enum/magic_enum.hpp>` header is unavailable
  - you might want to set this to avoid an accidental dependency on `magic_enum`

You can define these:

- with `#define`, before including `<magic_args/magic_args.hpp>`
- using compiler flags
- by creating a `"magic_args.tweaks.hpp"` and putting it in your include path
  - this is included *after* automatic configuration, allowing overrides
  - you can use `#undef` to make sure a macro *is not* set
  - you can use `#undef` followed by `#define` to make sure that a macro *is* set

For example, `my_project/include/magic_args.tweaks.hpp` might contain:

```c++
#ifdef __APPLE__
#undef MAGIC_ARGS_DISABLE_ICONV
#define MAGIC_ARGS_DISABLE_ICONV
#endif
```

This specific configuration is [not recommended](#macos), but is realistic for some users.

## `using namespace`

If you want to use `using namespace`, use `using namespace magic_args::public_api`; this
avoids pulling in the `magic_args::detail` namespace.

`magic_args` is designed to work easily regardless of whether you choose to use `using namespace` or not.
