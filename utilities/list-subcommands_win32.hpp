// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <Windows.h>
#include <expected>
#include <filesystem>
#include <memory>

struct FreeLibraryDeleter {
  void operator()(HMODULE library) const noexcept {
    FreeLibrary(library);
  }
};
using unique_library
  = std::unique_ptr<std::remove_pointer_t<HMODULE>, FreeLibraryDeleter>;

inline std::expected<unique_library, DWORD> open_library(
  const std::filesystem::path& path) {
  auto ret = unique_library {LoadLibraryExW(
    path.wstring().c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES)};
  if (ret) {
    return ret;
  }
  return std::unexpected {GetLastError()};
}

inline std::expected<const char*, DWORD> get_data_pointer(
  const unique_library& lib) {
  const auto ret = reinterpret_cast<const char*>(
    GetProcAddress(lib.get(), "magic_args_subcommands_list"));
  if (ret) {
    return ret;
  }
  return std::unexpected {GetLastError()};
}