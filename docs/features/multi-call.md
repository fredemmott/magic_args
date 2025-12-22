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