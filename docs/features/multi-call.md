---
layout: default
title: Multi-call
parent: Features
---

# Multi-call
{: .no_toc }

A multi-call binary is a single executable that changes its behavior based on the name it was invoked with (`argv[0]`). A famous example of this is `busybox`.

*magic_args* supports this via the same subcommand infrastructure used for [subcommands](subcommands.md), but instead of looking for the subcommand name as the first argument, it looks at the executable name.

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

## Quick Start: `MAGIC_ARGS_MULTI_CALL_MAIN`

For most users, the easiest and best way to create a multi-call binary is the `MAGIC_ARGS_MULTI_CALL_MAIN` macro. Each "command" is defined exactly like a [subcommand](subcommands.md#defining-subcommands).

```c++
#include <magic_args/magic_args.hpp>
#include <magic_args/subcommands.hpp>

#include <print>

struct CommandFoo {
  // The name of the executable: `foo` or `foo.exe`
  static constexpr auto name = "foo";

  struct arguments_type {
    static constexpr auto description = "Do the foo thing";
    std::string mBar;
  };

  static int main(arguments_type&& args) {
    std::println("Foo bar: {}", args.mBar);
    return 0;
  }
};

struct CommandBar {
  static constexpr auto name = "bar";
  struct arguments_type {
    static constexpr auto description = "Do the bar thing";
    bool mFlag {false};
  };

  static int main(const arguments_type& args) {
    if (args.mFlag) {
       std::println("Bar with flag!");
    }
    return 0;
  }
};

// If this binary is named `foo`, CommandFoo::main is called.
// If it's named `bar`, CommandBar::main is called.
MAGIC_ARGS_MULTI_CALL_MAIN(CommandFoo, CommandBar);
```

{: .note }
To use this, you would typically create symbolic links or hard links named `foo` and `bar`, both pointing at the compiled binary. symlinks are usual on Linux and macOS, while hard links tend to be more consistently available on Windows end-user systems.

Like the other `_MAIN` macros, this macro handles:
- standard setup like UTF-8 conversion and `--help`/`--version` handling.
- stripping path and extension from `argv[0]` to identify the command.

## Root Command Information

Just like with subcommands, you can provide global information by passing a "root info" struct as the first argument. This is especially useful for setting the overall description or changing the parsing style.

```c++
struct MyRootInfo {
  static constexpr auto description = "My awesome multi-call tool";
  using parsing_traits = magic_args::powershell_style_parsing_traits;
};

MAGIC_ARGS_MULTI_CALL_MAIN(MyRootInfo, CommandFoo, CommandBar);
```

## Automatically creating links

[`magic_args-enumerate-subcommands`](#listing-subcommands) has several options to help you create the required links:

- `--hardlinks DIRECTORY` - *recommended*: creates the directory if needed, then creates a hard link for each subcommand
- `--symlinks DIRECTORY`: creates the directory if needed, then creates a symbolic link for each subcommand

Hard links are recommended if everything is on the same filesystem, especially on Windows. Windows requires developer mode or special privileges for symlink support.

There are additional options that can help with build system integration:

- `--force`: overwrite files if they already exist
- `--text-file PATH`: subcommands are written to the specified text file, one-per-line
- `--stamp-file PATH`: this file is created or updated every time `enumerate-subcommands` completes without error; the other output files *may* be updated if an error occurs after partial success
- `--output-style list|quiet|cmake-install`: what to print on stdout; default is to `list` subcommands. `cmake-install` emulates cmake-install output format

The usual pattern is to use `--hardlinks`, `--stamp-file` in your build system, and `--text-file` to feed to a post-build installer.

### CMake integration

To automatically create links when building with CMake, use `magic_args_enumerate_subcommands()`:

```cmake
find_package(magic_args CONFIG REQUIRED)
include(MagicArgs)
magic_args_enumerate_subcommands(
  my_executable
  HARDLINKS "${CMAKE_CURRENT_BINARY_DIR}/my_executable-hardlinks/"
)
```

The following options are supported:

- `HARDLINKS path`: equivalent to `--hardlinks`
- `SYMLINKS path`: equivalent to `--symlinks`
- `TEXT_FILE path`: equivalent to `--text-file`
- `STAMP_FILE`: equivalent to `--stamp-file`

If you also use CMake `install()`:

```cmake
install(TARGETS my_executable) # The executable MUST also be installed
magic_args_install_multicall_links(
  my_executable
  RELATIVE_SYMLINKS
  SYMLINKS_DESTINATION "bin"
)
```

The following options are supported:

- `DESTINATION PATH`: same as `install(... DESTINATION PATH)`; should match your executable
- `COMPONENT NAME`: should match your executable's install `COMPONENT`, if it has one
- `SYMLINKS_DESTINATION PATH`: create symbolic links; path is relative to the installation root
- `HARDLINKS_DESTINATION PATH`: create hard links; path is relative to the installation root
- `RELATIVE_SYMLINKS`: if `SYMLINKS_DESTINATION` is specified, symlinks will be relative instead of absolute
- `STAMP_FILE`: equivalent to `--stamp-file`

## Manual Invocation

If you need more control, you can use `invoke_subcommands` or `parse_subcommands` with `magic_args::multicall_traits<Traits>`.

```c++
int main(int argc, char** argv) {
  const auto utf8_argv = magic_args::make_utf8_argv(argc, argv);
  if (!utf8_argv) {
    return EXIT_FAILURE;
  }

  // Use multicall_traits to tell magic_args to look at argv[0]
  auto result = magic_args::invoke_subcommands<
    magic_args::multicall_traits<>,
    CommandFoo,
    CommandBar>(*utf8_argv);

  if (result) {
    return result.value();
  }

  return magic_args::is_error(result.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
}
```

If you have a root info struct, you can use `magic_args::multicall_traits<RootInfo>`.

`invoke_subcommands_silent()`, `parse_subcommands()`, and `parse_subcommands_silent()` work just like [for normal subcommands](subcommands.md), as long as `multicall_traits<>` is used.

## Listing subcommands

End users can get a list of subcommands by invoking with an unrecognized command name (`argv[0]`).

An API and inspection tool are [also available](subcommands.md#listing-subcommands); this is especially useful for multi-call binaries, as you can use it in your build system to automatically create the required links (i.e. one link for each subcommand).