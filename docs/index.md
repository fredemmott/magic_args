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

MAGIC_ARGS_MAIN(const MyArgs& args) {
  magic_args::dump(args);
  return 0;
}
```

The macro is optional but takes care of [encoding issues](features/encoding.md) and error handling.

As well as UTF-8 conversion and argument parsing, this gets you a working `--help`:

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

See [setup](setup.md).

## License

*magic_args* is MIT-licensed.
