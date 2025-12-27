// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <dlfcn.h>
#include <expected>
#include <filesystem>

// Just used for unique_any
#if __has_include(<magic_args/detail/unique_any.hpp>)
#include <magic_args/detail/unique_any.hpp>
#else
// single-header
#include <magic_args/magic_args.hpp>
#endif

using unique_library = magic_args::detail::unique_any<void*, &dlclose>;

inline std::expected<unique_library, const char*> open_library(
  const std::filesystem::path& path) {
  auto ret = unique_library {dlopen(path.string().c_str(), RTLD_LAZY)};
  if (ret) {
    return ret;
  }
  return std::unexpected {dlerror()};
}

inline std::expected<const char*, const char*> get_data_pointer(
  const unique_library& lib) {
  const auto ret = reinterpret_cast<const char*>(
    dlsym(lib.get(), "magic_args_subcommands_list"));
  if (ret) {
    return ret;
  }
  return std::unexpected {dlerror()};
}