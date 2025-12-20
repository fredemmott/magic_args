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

**magic_args** is available in the main [vcpkg][] registry as `magic-args`.

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

If you are using vcpkg, the `charset-conversion` feature (enabled by default) enables the recommended path (linking against `iconv`); you don't need to do anything.

Otherwise, to use *magic_args* on macOS, you must choose between three options:

- *recommended*: link against `iconv`: the program will convert encodings as necessary. This option is recommended because it gives you consistent behavior on all major platforms
- define `MAGIC_ARGS_DISABLE_ICONV`: if the character set is not UTF-8 or a compatible 7-bit encoding (e.g. US-ASCII), an error will be reported. This is slightly simpler, and may be acceptable as non-UTF-8 locales are extremely rare on macOS
- handle (or ignore) encoding issues yourself, and call the *magic_args* functions directly instead of using the `MAGIC_ARGS*_MAIN()` macros

If you are using CMake but not using vcpkg, you can follow the recommended path with the following code:

```cmake
if (APPLE)
  find_package(Iconv REQUIRED)
  target_link_libraries(my_executable PRIVATE Iconv::Iconv)
endif ()
```

This does not affect other platforms:

- on Windows, the Win32 API is used instead of `iconv`
- on Linux, `iconv` is typically provided by libc and doesn't require explicit linking

## CMake Configuration

Most users should use [vcpkg][], and not need to change anything here.

If you are using CMake, *magic_args* recognizes the following variables:

- `ENABLE_ICONV=manual|not-windows|OFF|ON`
  - `not-windows` (*recommended*): uses Win32 APIs for encoding conversion on Windows, and `iconv` on other platforms;
    - this value is set by the vcpkg `charset-conversion` feature (enabled by default)
    - on some platforms (e.g. macOS), this will introduce a dependency on libiconv
    - on Linux, iconv is typically part of libc, so this will not introduce an additional dependency
  - `manual` (*default*): iconv is used if `<iconv.h>` is available, but it is not automatically linked
    - this will 'just work' on most Windows environments because `<iconv.h>` is not typically available
    - this will 'just work' on most Linux environments, as iconv is typically part of libc
    - on macOS and some rare Linux configurations, users will need to either link against iconv themselves, or
      define `MAGIC_ARGS_DISABLE_ICONV`
    - this is the default because on macOS (and some rare Linux environments), `'not-windows'` introduces an additional
      link dependency
    - the vcpkg default differs because on-by-default-but-optional dependencies are a commonly used feature of vcpkg,
      rather than a potential surprise
  - `ON`: iconv is always added as a dependency, even on Windows
  - `OFF`: iconv is never added as a dependency, and `MAGIC_ARGS_DISABLE_ICONV` is defined
- `ENABLE_MAGIC_ENUM=auto|OFF|ON`
  - `auto` (*default*): [*magic_enum*][magic_enum] is used if CMake `find_package()` is able to find `magic_enum`
  - `ON`: `magic_enum` will be used; CMake will fail if `find_package()` fails
  - `OFF`: CMake will not look for `magic_enum`, and `MAGIC_ARGS_DISABLE_ENUM` will be set

## Preprocessor Configuration

Most users should use [vcpkg][vcpkg], and not need to change anything here.

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

This specific configuration is [not recommended](features/encoding.md#macos), but is realistic for some users.

## `using namespace`

If you want to use `using namespace`, use `using namespace magic_args::public_api`; this
avoids pulling in the `magic_args::detail` namespace.

`magic_args` is designed to work easily regardless of whether you choose to use `using namespace` or not.

[vcpkg]: https://vcpkg.io/
[magic_enum]: https://github.com/Neargye/magic_enum