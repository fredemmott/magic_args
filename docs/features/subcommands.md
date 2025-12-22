---
layout: default
title: Subcommands
parent: Features
---

# Subcommands
{: .no_toc }

*magic_args* supports subcommands which accept different arguments; you might know this concept from `git` (e.g. `git push`, `git commit`).

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

## Quick Start: `MAGIC_ARGS_SUBCOMMANDS_MAIN`

For most users, the easiest and best way to use subcommands is the `MAGIC_ARGS_SUBCOMMANDS_MAIN` macro. Each subcommand is defined as a struct.

```c++
#include <magic_args/magic_args.hpp>
#include <magic_args/subcommands.hpp>

struct CommandFoo {
  // The name used on the command line: `myapp foo`
  static constexpr auto name = "foo";

  // The arguments for this specific subcommand
  struct arguments_type {
    // optional,
    static constexpr auto description = "Do the foo thing";
    std::string mBar;
  };

  // The function to run if this subcommand is selected
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

MAGIC_ARGS_SUBCOMMANDS_MAIN(CommandFoo, CommandBar);
```

The macro expands into a standard `int main(int argc, char** argv)` (or `int wmain(int argc, wchar_t** argv)` on
Windows) which handles:
- converting argv to UTF-8
- parsing arguments
- displaying `--help`, `--version`, and any usage error messages
- invoke the appropriate subcommand

## Defining subcommands

A subcommand is any type that meets these requirements:

1.  **`name`**: A `static constexpr` string (or string-like) member named `name`.
2.  **`arguments_type`**: A nested struct defining the arguments for the subcommand. This follows the same rules as [regular argument structs](../index.md).
3.  **`main`**: A `static` method that takes an `arguments_type` (or a reference/move-reference)

## Root command information

You can optionally provide global information (like the top-level description and version) by creating a "root info" struct and passing it as the first argument to the subcommand functions.

```c++
struct MyRootInfo {
  static constexpr auto description = "My awesome multi-tool";
  static constexpr auto version = "1.2.3";
  // You can also specify parsing_traits here, e.g. for PowerShell style
  // using parsing_traits = magic_args::powershell_style_parsing_traits;
};

// When using the macro, the root info is optional and comes first:
MAGIC_ARGS_SUBCOMMANDS_MAIN(MyRootInfo, CommandFoo, CommandBar);
```

## Alternatives to the macro

If you need more control (for example, if you want to use the return value of the subcommand's `main` in your own logic), you can use `invoke_subcommands` or `parse_subcommands`.

These functions should be combined with `magic_args::make_utf8_argv()` to make sure that [encoding is handled correctly](encoding.md).

### `invoke_subcommands`

`invoke_subcommands` parses the command line, identifies the subcommand, and calls its `main` method. It returns an `std::expected<int, ...>`.

```c++
int main(int argc, char** argv) {
  const auto utf8_argv = magic_args::make_utf8_argv(argc, argv);
  if (!utf8_argv) {
    return EXIT_FAILURE;
  }
    
  // MyRootInfo is optional; see "Root Command Information" above
  auto result = magic_args::invoke_subcommands<MyRootInfo, CommandFoo, CommandBar>(
    *utf8_argv);
  
  if (result) {
    return result.value();
  }
 
  // `--help` and `--version` will give you an unexpected `result`, but are not errors
  return magic_args::is_error(result.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
}
```

On Windows, you can also use `make_utf8_argv()` with a `wchar_t**` argv (e.g. from `wmain()`), or with no arguments to use `GetCommandLineW()`.

{: .note }
If parsing is incomplete (either due to an error, or being passed `--help` or `--version`), `invoke_subcommands()` will print the relevant messages to `stdout`/`stderr`; use `invoke_subcommands_silent()` instead if you do not want this behavior.

### `parse_subcommands`

`parse_subcommands` only handles the parsing and returns an `std::expected<std::variant<subcommand_match<T>...>, ...>`. This allows you to identify which subcommand was selected while accessing its specific arguments.

```c++
// MyRootInfo is optional; see "Root Command Information" above
auto result = magic_args::parse_subcommands<MyRootInfo, CommandFoo, CommandBar>(utf8_args);
if (result) {
  return std::visit(overload {
    [](magic_args::subcommand_match<CommandFoo>& match) {
      // match->mBar accesses CommandFoo::arguments_type
      std::println("Executing Foo with bar={}", match->mBar);
    },
    [](magic_args::subcommand_match<CommandBar>& match) {
      std::println("Executing Bar, flag is {}", match->mFlag);
    },
  }, *result);
}
```

`subcommand_match<T>` acts as a wrapper around `T::arguments_type`. You can access the arguments using `operator->` or `value()`.

Combining `std::visit` with the overload pattern is generally the best way to implement both the success dispatch, and the handling of `result.error()`; while *magic_args* does not provide an implementation, it can be implemented like this in C++23:

```c++
template<class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
```

As it's an `std::variant`, you can also use other approaches like `std::holds_alternative`, `std::get`, and `std::get_if`.

{: .note }
If parsing is incomplete (either due to an error, or being passed `--help` or `--version`), `parse_subcommands()` will print the relevant messages to `stdout`/`stderr`; use `parse_subcommands_silent()` instead if you do not want this behavior.
