// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_ENCODING_HPP
#define MAGIC_ARGS_DETAIL_ENCODING_HPP
#include <expected>
#include <ranges>
#include <span>
#include <string_view>
#include <system_error>
#include <variant>

#if __has_include(<langinfo.h>)
#include <langinfo.h>
// Needed on macOS and BSD
#if __has_include(<xlocale.h>)
#include <xlocale.h>
#endif
#endif

namespace magic_args::detail {

struct unix_like_platform_t {};
struct linux_platform_t : unix_like_platform_t {};
struct macos_platform_t : unix_like_platform_t {};
struct win32_platform_t {};
#ifdef _WIN32
using current_platform_t = win32_platform_t;
#elifdef __APPLE__
using current_platform_t = macos_platform_t;
#elifdef __linux__
using current_platform_t = linux_platform_t;
#else
using current_platform_t = unix_like_platform_t;
#endif

}// namespace magic_args::detail

namespace magic_args::inline public_api {

// Null, argc = 0, etc
struct invalid_parameter_t {
  constexpr bool operator==(const invalid_parameter_t&) const noexcept
    = default;
};
struct encoding_not_supported_t {
  constexpr bool operator==(const encoding_not_supported_t&) const noexcept
    = default;
};
struct encoding_conversion_failed_t {
  std::error_code mPlatformErrorCode;
  constexpr bool operator==(const encoding_conversion_failed_t&) const noexcept
    = default;
};

using make_utf8_argv_error_t = std::variant<
  invalid_parameter_t,
  encoding_not_supported_t,
  encoding_conversion_failed_t>;
static_assert(std::equality_comparable<make_utf8_argv_error_t>);
}// namespace magic_args::inline public_api

namespace magic_args::detail {
#if __has_include(<langinfo.h>)
// Not using `unique_ptr` because:
// - `locale_t` is not guaranteed to be a pointer
// - even when it is, it can be `void*` (e.g. on macos)
struct unique_locale {
  unique_locale() = delete;
  unique_locale(const unique_locale&) = delete;
  unique_locale(unique_locale&&) = delete;
  unique_locale& operator=(const unique_locale&) = delete;
  unique_locale& operator=(unique_locale&&) = delete;

  unique_locale(locale_t locale) : mLocale(locale) {
  }

  ~unique_locale() {
    if (mLocale) {
      freelocale(mLocale);
    }
  }

  locale_t get() const noexcept {
    return mLocale;
  }

  operator bool() const noexcept {
    return mLocale;
  }

 private:
  locale_t mLocale {};
};

template <class TPlatform>
struct encoding_traits {
  static bool process_argv_are_utf8() noexcept {
    const unique_locale locale {newlocale(LC_CTYPE_MASK, "", (locale_t)0)};
    if (!locale) {
      // ... Let's not just break the app in this weird edge case
      return true;
    }
    const std::string_view encoding {nl_langinfo_l(CODESET, locale.get())};
    // These 7-bit encodings are also always valid UTF-8
    return encoding == "UTF-8" || encoding == "US-ASCII"
      || encoding == "ANSI_X3.4-1968";
  }
  static auto make_utf8_argv(const int argc, const char* const* argv)
    -> std::expected<
      decltype(std::views::counted(argv, argc)),
      make_utf8_argv_error_t> {
    if (!process_argv_are_utf8()) {
      return std::unexpected {encoding_not_supported_t {}};
    }
    return std::views::counted(argv, argc);
  }
};
#else
// Enable the windows extensions for windows support
template <class TPlatform>
struct encoding_traits {};
#endif
}// namespace magic_args::detail

namespace magic_args::inline public_api {

template <class Platform = detail::current_platform_t>
auto make_utf8_argv(const int argc, const char* const* argv) {
  return detail::encoding_traits<Platform>::make_utf8_argv(argc, argv);
}
}// namespace magic_args::inline public_api

#endif
