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
  // this gets you an `std::expected<MyArgs, magic_args::incomplete_parse_reason_t>`,
  // but assumes that argv is UTF-8
  const auto args = magic_args::parse<MyArgs>(std::views::counted(argv, argc));
  if (args) {
      // Your code here. In this case, we'll just print the struct
      magic_args::dump(*args);
      return EXIT_SUCCESS;
  }
    // This could be an actual error, e.g. invalid argument,
    // or something like `--help` or `--version`, which while not an error,
    // are an 'unexpected' outcome in the std::expected
    return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
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
