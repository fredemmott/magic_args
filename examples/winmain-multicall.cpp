// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
// Support single-header version
#if __has_include(<magic_args/subcommands.hpp>)
#include <magic_args/subcommands.hpp>
#endif

#include <Windows.h>
#include "win32_utf8_messagebox.hpp"

struct CommandFooBar {
  struct arguments_type {
    std::string mBar;
    std::string mBaz;
  };
  static int main(arguments_type&& args) {
    magic_args::capturing_console_output output;
    output.out.println("in CommandFooBar::main");
    magic_args::dump(args, output.out);
    utf8_messagebox(output.out_str(), MB_ICONINFORMATION);
    return EXIT_SUCCESS;
  }
};

struct CommandHerp {
  struct arguments_type {
    static constexpr auto description = "Do the derpy thing";
    static constexpr auto version = "Herp v1.2.3";

    std::string mDerp;
  };

  static int main(arguments_type&& args) {
    magic_args::capturing_console_output output;
    output.out.println("in CommandHerp::main");
    magic_args::dump(args, output.out);
    utf8_messagebox(output.out_str(), MB_ICONINFORMATION);
    return EXIT_SUCCESS;
  }
};

struct Root {
  using subcommands
    = magic_args::invocable_subcommands_list<CommandFooBar, CommandHerp>;

  static int main(
    std::expected<
      int,
      magic_args::subcommands_winmain_unexpected_t<subcommands>>&& ok,
    [[maybe_unused]] HINSTANCE instance,
    [[maybe_unused]] int nCmdShow) {
    if (ok) [[likely]] {
      return *ok;
    }

    utf8_messagebox(
      ok.error().output,
      is_error(ok.error()) ? MB_ICONERROR : MB_ICONINFORMATION);

    return is_error(ok.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }
};

MAGIC_ARGS_MULTICALL_WINMAIN(Root);