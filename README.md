# magic_args

*magic_args* is a C++23 header-only library for command-line argument handling. Its primary goal is ease of use, while
also aiming to be full-featured.

## Example

```c++
#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool mFoo {false};
  std::string mBar;
  int mBaz {0};
};

int main(int argc, char** argv) {
  // This gets you an
  // std::expected<MyArgs, magic_args::incomplete_parse_reason>
  const auto args = magic_args::parse<MyArgs>(argc, argv);
  
  if (!args.has_value()) {
    if (args.error() == magic_args::HelpRequested) {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}
```

```
> minimal.exe --help
Usage: minimal [OPTIONS...]

Options:

      --foo
      --bar=VALUE
      --baz=VALUE

  -?, --help                   show this message
> minimal.exe --foo --baz=123 --bar SomeString
mFoo                          `true`
mBar                          `SomeString`
mBaz                          `123`
```

[Help text, short flags, and positional arguments are also supported](examples/everything.cpp).

Long names are inferred for struct members in the following forms:

```c++
struct MyArgs {
  std::string mEmUpperCamel;
  std::string m_EmUnderscoreUpperCamel;
  std::string _UnderscoreUpperCamel;
  std::string _underscoreLowerCamel;
  std::string UpperCamel;
  std::string lowerCamel;
  std::string m_em_snake_case;
  std::string snake_case;
};
```

## Requirements

*magic_args* requires C++23, and is tested with:

- latest Visual Studio cl.exe and clang-cl.exe
- latest Apple Clang (XCode)
- GCC 13 on Ubuntu 24.04

The *magic_args* library has no other dependencies; CMake, Catch2, and vcpkg are used for the examples and unit tests.

## Using *magic_args* in your project

Add the `magic_args` directory to your project and include path, using your preferred method. Options include:

- a git submodule
- CMake `FetchContent` and `ExternalProject_Add`
- adding the files manually

A single-header-file version is also
available [from the releases page](https://github.com/fredemmott/magic_args/releases/latest).

If you want to use `using namespace`, use `using namespace magic_args::public_api`; this avoids pulling in the
`magic_args::detail` namespace.

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
- support for `WinMain` and `wWinMain`

### Customizing options

Long arguments are automatically inferred for struct members; if you want to use a different argument name, help text,
or to support short flags, use the `magic_args::option<>` and `magic_args::flag<>` types:

```c++
struct MyArgs {
  magic_args::flag mMyFlag {
    .mName = "long-name", // --my-flag
    .mHelp = "documentation here",
    .mShortName = "m", // -m
  };
  magic_args::option<std::string> mOpt1 {
    .mValue = "default",
    .mName = "option", // --option
    .mHelp = "documentation here",
    .mShortName = "o", // -m
  };
  magic_args::option<std::string> mOpt2 { {}, "option-2", "documentation", "p" };
};
```

Option and flag members are implicitly convertible to their template type (or `bool` for flags). `has_value()`,
`value()`, `operator*()`, and `operator->()` are supported for `std::optional<>` types.

### Positional arguments

```c++
struct MyArgs {
  magic_args::mandatory_positional_argument<std::string> mPos1;
  magic_args::optional_positional_argument<std::string> mPos2;
  magic_args::optional_positional_argument<std::vector<std::string>> mPos3;
};
```

- multiple `std::vector<>` arguments are not permitted
- if present, an `std::vector<>` argument must be the last argument
- if both mandatory and optional positional arguments are present, all optional positional arguments must be after all
  mandatory positional arguments

Default values, names, and documentation can be specified in the same way as flags and options:

```c++
struct MyArgs {
  magic_args::mandatory_positional_argument<std::string> mPos1 {
    .mValue = "default",
    .mName = "POSITIONAL",
    .mHelp = "help text here",
  };
 };
```

### Program information

A description, examples, and version information can be provided; description and examples will be shown in `--help`,
and version information will be used for `--version`:

```c++
const magic_args::program_info programInfo {
  .mDescription = "My program does something",
  .mVersion = "HerpDerp v1.0.0",
  .mExamples = {
    "myprog FOO",
    "myprog FOO --bar",
  },
};
const auto args = magic_args::parse<MyArgs>(argc, argv, programInfo);
```

```
> foo --help
Usage: foo [OPTIONS...] INPUT
My program does something.

Examples:

  myprog FOO
  myprog FOO --bar
 
Options:

  -b, --bar                   help text here
      
  -?, --help                  show this message
      --version               print program version

Arguments:

      INPUT                   help text here
```

### Custom argument types

Types can be supported by implementing support for `operator >>` from a stream; alternatively, implement the following
functions in the same namespace as your type:

```c++
// Used by `magic_args::parse()`
void from_string_argument(T& v, std::string_view arg);
// Used by `magic_args::dump()`; alternatively, implement `std::formatter<>`
auto formattable_argument_value(const T& v);
```

### Support for `WinMain` and `wWinMain`

If possible, use a standard `main` function instead. *magic_args* includes helpers for when that is impractical:

```c++
#define MAGIC_ARGS_ENABLE_WINDOWS_EXTENSIONS
#include <magic_args/magic_args.hpp>
// If you're not using the single-header version, you can do this instead of using the macro:
#include <magic_args/windows.hpp>

...
magic_args::attach_to_parent_terminal();
const auto args = magic_args::parse<MyArgs>(GetCommandLineW());
```

When using these helpers, arguments are converted to
UTF-8; [using UTF-8 as your process code page](tests/utf8-process-code-page.manifest) is *strongly* recommended.

`GetCommandLineW()` should be used instead of the `lpCmdLine` parameter as it includes the program path (`argv[0]`);
using `lpCmdLine` leads to inconsistent results, as the behavior of `CommandLineToArgvW()` varies when using
`lpCmdLine`.

The Windows helpers are not enabled by default as they include `<Windows.h>`.

### Powershell-like syntax

You can choose to use powershell-like syntax instead of the default GNU-like syntax:

```c++
const auto args = magic_args::parse<
  MyArgs,
  magic_args::powershell_style_parsing_traits>(argc, argv);
```

This will use single dashes for arguments, and `UpperCamelCase` for inferred names:

```
Usage: minimal-powershell-style [OPTIONS...]

Options:

      -Foo
      -Bar=VALUE
      -Baz=VALUE

  -?, -Help                    show this message
```

## License

*magic_args* is [MIT-licensed](LICENSE).