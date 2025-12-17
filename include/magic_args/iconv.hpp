// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#if ( \
  defined(MAGIC_ARGS_ENABLE_ICONV_EXTENSIONS) \
  || !defined(MAGIC_ARGS_SINGLE_FILE)) \
  && !defined(MAGIC_ARGS_ICONV_HPP)
#define MAGIC_ARGS_ICONV_HPP

#define MAGIC_ARGS_HAVE_ICONV_EXTENSIONS
#define MAGIC_ARGS_CAN_CONVERT_TO_UTF8

#include <cerrno>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <iconv.h>

#ifndef MAGIC_ARGS_SINGLE_FILE
#include "detail/encoding.hpp"
#endif

namespace magic_args::detail::iconv {
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

inline std::expected<std::vector<std::string>, make_utf8_argv_error_t>
make_utf8_argv(
  const int argc,
  const char* const* argv,
  const std::string& charset) {
  if (argc < 0 || (argc > 0 && argv == nullptr)) {
    return std::unexpected {invalid_parameter_t {}};
  }

  using TEncoding = encoding_traits<unix_like_platform_t>;
  if (TEncoding::process_argv_are_utf8()) {
    // TODO: validate
    return std::vector<std::string> {argv, argv + argc};
  }

  if (charset.empty()) {
    return std::unexpected {encoding_not_supported_t {}};
  }

  const unique_any<iconv_t, &iconv_close, decltype([](const auto p) {
                     return p != reinterpret_cast<iconv_t>(-1);
                   })>
    converter {iconv_open(charset.c_str(), "UTF-8")};
  if (!converter) {
    return std::unexpected {encoding_conversion_failed_t {
      std::error_code {errno, std::system_category()}}};
  }

  std::vector<std::string> ret;
  ret.reserve(static_cast<size_t>(argc));

  std::string converted;
  converted.reserve(1024);
  for (int i = 0; i < argc; ++i) {
    const std::string_view arg {argv[i] ? argv[i] : ""};
    if (const auto ok = convert_with_iconv(converted, arg, converter.get());
        !ok) {
      return std::unexpected {encoding_conversion_failed_t {ok.error()}};
    }
    ret.emplace_back(converted);
  }
  return ret;
}

}// namespace magic_args::detail::iconv

// Provide the platform specialization that delegates to the public API above
template <>
struct magic_args::detail::encoding_traits<
  magic_args::detail::current_platform_t>
  : encoding_traits<unix_like_platform_t> {
  // static constexpr bool can_convert_to_utf8 = true;
  static std::expected<std::vector<std::string>, make_utf8_argv_error_t>
  make_utf8_argv(const int argc, const char* const* argv) {
    return iconv::make_utf8_argv(argc, argv, environment_charset());
  }
};

#endif// MAGIC_ARGS_ICONV_HPP
