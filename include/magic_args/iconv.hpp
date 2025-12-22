// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_ICONV_HPP
#define MAGIC_ARGS_ICONV_HPP

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/config.hpp"
#include "detail/encoding.hpp"
#endif

#ifndef MAGIC_ARGS_DISABLE_ICONV

#include <cerrno>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <iconv.h>

namespace magic_args::detail {
inline std::expected<void, std::error_code> convert_with_iconv(
  std::string& out,
  const std::string_view in,
  const iconv_t converter) {
  out.clear();
  if (in.empty()) {
    return {};
  }

  // Try a reasonable initial capacity; will grow if needed
  out.resize(in.size() * 4 + 16);

  const char* pin = in.data();
  size_t inLeft = in.size();
  char* outBase = out.data();
  size_t outLeft = out.size();

  while (true) {
    char* inBuf = const_cast<char*>(pin);
    const auto res = ::iconv(converter, &inBuf, &inLeft, &outBase, &outLeft);
    pin = inBuf;

    if (res != (size_t)-1) {
      break;// ssuccess
    }

    if (errno == E2BIG) {
      // Need more output space; grow by 2x and continue
      const auto used = static_cast<size_t>(out.data() + out.size() - outBase);
      const auto produced = out.size() - outLeft - used;
      (void)produced;// silence unused warning in some builds

      const auto alreadyWritten = out.size() - outLeft;
      const auto oldSize = out.size();
      out.resize(oldSize * 2);
      outBase = out.data() + alreadyWritten;
      outLeft = out.size() - alreadyWritten;
      continue;
    }

    const auto ec = std::error_code {errno, std::system_category()};
    return std::unexpected {ec};
  }

  // Shrink to actual size written
  const auto written = out.size() - outLeft;
  out.resize(written);
  return {};
}

constexpr bool iconv_t_is_valid(const iconv_t p) noexcept {
  return p && p != reinterpret_cast<iconv_t>(-1);
};

using unique_iconv_t = unique_any<iconv_t, &iconv_close, &iconv_t_is_valid>;

inline std::expected<std::vector<std::string>, make_utf8_argv_error_t>
make_utf8_argv(
  const int argc,
  const char* const* argv,
  const std::string_view fromCharset,
  const iconv_t converter) {
  std::vector<std::string> ret;
  ret.reserve(static_cast<size_t>(argc));

  std::string converted;
  converted.reserve(1024);
  for (int i = 0; i < argc; ++i) {
    const std::string_view arg {argv[i] ? argv[i] : ""};
    if (const auto ok = detail::convert_with_iconv(converted, arg, converter);
        !ok) [[unlikely]] {
      return std::unexpected {
        encoding_conversion_failed_t {std::string {fromCharset}, ok.error()}};
    }
    ret.emplace_back(converted);
  }
  return ret;
}

}// namespace magic_args::detail

namespace magic_args::inline public_api {

inline std::expected<std::vector<std::string>, make_utf8_argv_error_t>
make_utf8_argv(
  const int argc,
  const char* const* argv,
  const std::string& charset) {
  if (argc < 0 || (argc > 0 && argv == nullptr)) {
    return std::unexpected {invalid_parameter_t {}};
  }

  if (charset.empty()) {
    return std::unexpected {invalid_parameter_t {}};
  }

  using TEncoding = detail::encoding_traits<detail::unix_like_platform_t>;
  if (TEncoding::is_utf8(charset)) {
    // Match Windows behavior: require valid UTF-8
    // This happens on Windows because it gets converted to UTF-16
    const detail::unique_iconv_t validator {iconv_open("UTF-8", "UTF-8")};
    if (!validator) {
      // ... iconv is busted, might as well let the program work
      return std::vector<std::string> {argv, argv + argc};
    }
    return detail::make_utf8_argv(argc, argv, "UTF-8", validator.get());
  }

  const detail::unique_iconv_t converter {iconv_open("UTF-8", charset.c_str())};
  if (!converter) {
    return std::unexpected {encoding_not_supported_t {
      charset, std::error_code {errno, std::system_category()}}};
  }

  return detail::make_utf8_argv(argc, argv, charset, converter.get());
}
}// namespace magic_args::inline public_api

// Provide the platform specialization that delegates to the public API above
template <>
struct magic_args::detail::encoding_traits<
  magic_args::detail::current_platform_t>
  : encoding_traits<unix_like_platform_t> {
  // static constexpr bool can_convert_to_utf8 = true;
  static std::expected<std::vector<std::string>, make_utf8_argv_error_t>
  make_utf8_argv(const int argc, const char* const* argv) {
    return public_api::make_utf8_argv(argc, argv, environment_charset());
  }
};

#endif
#endif