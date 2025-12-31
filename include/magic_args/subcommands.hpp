// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_SUBCOMMANDS_HPP
#define MAGIC_ARGS_SUBCOMMANDS_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "main_macros.hpp"
#include "subcommands/inspection.hpp"
#include "subcommands/invoke_multicall.hpp"
#include "subcommands/invoke_subcommands.hpp"
#include "subcommands/is_error.hpp"
#include "subcommands/main_macros.hpp"
#include "subcommands/parse_subcommands.hpp"
#include "subcommands/parse_subcommands_silent.hpp"
#if _WIN32
#include "windows/multicall_winmain.hpp"
#include "windows/subcommands_winmain.hpp"
#endif
#endif

#endif