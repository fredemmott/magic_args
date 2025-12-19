// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_CONFIG_HPP
#define MAGIC_ARGS_DETAIL_CONFIG_HPP

#ifdef _WIN32
#undef MAGIC_ARGS_DISABLE_ICONV
#define MAGIC_ARGS_DISABLE_ICONV
#endif

#if !__has_include(<magic_enum/magic_enum.hpp>)
#undef MAGIC_ARGS_DISABLE_ENUM
#define MAGIC_ARGS_DISABLE_ENUM
#endif

// Override any of the above by creating this file
// and putting it in your compiler's include path
#if __has_include(<magic_args.tweaks.hpp>)
#include <magic_args.tweaks.hpp>
#endif

#endif
