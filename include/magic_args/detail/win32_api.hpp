// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_WIN32_API_HPP
#define MAGIC_ARGS_DETAIL_WIN32_API_HPP
#ifdef _WIN32

/* Minimal Win32 API declarations to avoid pulling in `<Windows.h>` and
 * `<shellapi.h>`.
 *
 * This makes it reasonable to always include Windows implementations of
 * core functionality (like charset conversions) - without impacting portable
 * code.
 *
 * Pulling in the Windows headers would be problematic because:
 *
 * - they're *huge* (slow compilation)
 * - they define a load of macros which can be harmful to portable code,
 *   especially `min` and `max`
 * - this is a header-only library, so the Windows headers would be pulled into
 *   downstream code
 *
 * While users can minimize these issues with definitions like
 * NOMINMAX, WIN32_LEAN_AND_MEAN, we're aiming for a portable and good
 * experience by default, without requiring that kind of knowledge or setup
 */

namespace magic_args::detail::inline win32_definitions {

using WIN32_BOOL = int;
using WIN32_HANDLE = void*;

inline constexpr unsigned int WIN32_CP_UTF8 = 65001u;
inline constexpr unsigned int WIN32_CP_ACP = 0u;
inline constexpr unsigned long WIN32_WC_ERR_INVALID_CHARS = 0x00000080UL;
inline constexpr unsigned long WIN32_MB_ERR_INVALID_CHARS = 0x00000008UL;
inline constexpr unsigned long WIN32_STD_INPUT_HANDLE
  = static_cast<unsigned long>(-10);
inline constexpr unsigned long WIN32_STD_OUTPUT_HANDLE
  = static_cast<unsigned long>(-11);
inline constexpr unsigned long WIN32_STD_ERROR_HANDLE
  = static_cast<unsigned long>(-12);
inline constexpr unsigned long WIN32_ATTACH_PARENT_PROCESS
  = static_cast<unsigned long>(-1);
inline const WIN32_HANDLE WIN32_INVALID_HANDLE_VALUE
  = reinterpret_cast<WIN32_HANDLE>(static_cast<intptr_t>(-1));

}// namespace magic_args::detail::inline win32_definitions

extern "C" __declspec(dllimport) unsigned long __stdcall GetLastError();
extern "C" __declspec(dllimport) int __stdcall WideCharToMultiByte(
  unsigned int CodePage,
  unsigned long dwFlags,
  const wchar_t* lpWideCharStr,
  int cchWideChar,
  char* lpMultiByteStr,
  int cbMultiByte,
  const char* lpDefaultChar,
  int* lpUsedDefaultChar);
extern "C" __declspec(dllimport) int __stdcall MultiByteToWideChar(
  unsigned int CodePage,
  unsigned long dwFlags,
  const char* lpMultiByteStr,
  int cbMultiByte,
  wchar_t* lpWideCharStr,
  int cchWideChar);
extern "C" __declspec(dllimport) wchar_t** __stdcall CommandLineToArgvW(
  const wchar_t*,
  int*);
extern "C" __declspec(dllimport) wchar_t* __stdcall GetCommandLineW();
extern "C" __declspec(dllimport) void* __stdcall LocalFree(void* hMem);

extern "C" __declspec(dllimport) magic_args::detail::WIN32_BOOL __stdcall
AttachConsole(unsigned long dwProcessId);
extern "C" __declspec(dllimport) magic_args::detail::WIN32_HANDLE __stdcall
GetStdHandle(unsigned long nStdHandle);
extern "C" __declspec(dllimport) unsigned int __stdcall GetACP();
#endif
#endif
