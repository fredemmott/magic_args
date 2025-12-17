---
layout: default
title: Home
nav_order: 1
---

# magic_args

*magic_args* is a C++23 header-only library for command-line argument handling. Its primary goal is ease of use, while
also aiming to be full-featured.

## Example

```c++
#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool foo {false};
  std::string bar;
  int baz {0};
};

int main(int argc, char** argv) {
  // this gets you an `std::expected<MyArgs,
  // magic_args::incomplete_parse_reason_t>`
  const auto args = magic_args::parse<MyArgs>(argc, argv);
  if (!args.has_value()) {
    // This could be an actual error, e.g. invalid argument,
    // or something like `--help` or `--version`, which while not an error,
    // are an 'unexpected' outcome in the std::expected
    return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}
```

This gets you:

```
> ./example-minimal.exe --help
Usage: example-minimal [OPTIONS...]

Options:

      --foo
      --bar=VALUE
      --baz=VALUE

  -?, --help                   show this message
> ./example-minimal.exe --foo --bar=someValue --baz=42
foo                           `true`
bar                           `someValue`
baz                           `42`
```

Struct members are automatically converted to `--foo-bar` format; magic_args will recognize and convert the following
conventions:

```c++
struct MyArgs {
  T mEmUpperCamel;
  T m_EmUnderscoreUpperCamel;
  T _UnderscoreUpperCamel;
  T _underscoreLowerCamel;
  T UpperCamel;
  T lowerCamel;
  T m_em_snake_case;
  T snake_case;
};
```

## Features

- `--foo` and `-f` for flags (bool options that default to false and can be set to false)
- `--foo=bar`, `--foo bar`, and `-f bar` syntax for options with values
- `--version` and `--help`
- examples and descriptive text can be added to `--help`
- alternative powershell-style syntax
- compatible with `std::optional<>` used when the difference between 'not provided' and 'default value' is important
- positional arguments
- mandatory positional arguments
- positional arguments with multiple values
- `--`, treating all later arguments as positional arguments, even if they match an option
- win32 helpers for `winMain()` and `wWinMain()` programs

## Requirements

*magic_args* requires C++23, and is tested with:

- Visual Studio 2022 - cl
- Visual Studio 2022 - clang-cl
- latest Apple Clang (XCode)
- GCC 14 on Ubuntu 24.04

The *magic_args* library has no other dependencies; CMake, Catch2, and vcpkg are used for the examples, but are not
required.

## Using *magic_args* in your project

See [getting started](getting-started.md).

## License

*magic_args* is MIT-licensed.
