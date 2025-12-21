// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP
#define MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP

#include <algorithm>
#include <ranges>
#include <utility>

namespace magic_args::detail::constexpr_strings {

// These should all be consteval, but under VS 2022 we hit lifetime bugs :(

[[nodiscard]]
constexpr bool is_upper(const char c) {
  return c >= 'A' && c <= 'Z';
}
[[nodiscard]]
constexpr bool is_lower(const char c) {
  return c >= 'a' && c <= 'z';
}
[[nodiscard]]
constexpr char to_lower(const char c) {
  if (is_upper(c)) {
    return c + ('a' - 'A');
  }
  return c;
}
[[nodiscard]]
constexpr char to_upper(const char c) {
  if (is_lower(c)) {
    return c - ('a' - 'A');
  }
  return c;
}

struct normalizer_t {
  template <class Self>
    requires requires { Self::buffer; }
  constexpr operator std::string_view(this const Self&) noexcept {
    return std::string_view {Self::buffer};
  }

  template <std::equality_comparable_with<std::string_view> T, class Self>
    requires requires { Self::buffer; }
  constexpr bool operator==(this const Self&, T&& other) noexcept {
    return std::string_view {Self::buffer} == std::forward<T>(other);
  }
};

template <auto Name>
struct hyphenate_t : normalizer_t {
  static_assert(!std::string_view {Name}.empty());
  static consteval std::size_t count() {
    std::size_t size = 1;
    bool prev_was_underscore = false;
    for (auto&& c: std::string_view {Name}.substr(1)) {
      if (c == '_') {
        prev_was_underscore = true;
        continue;
      }
      if (std::exchange(prev_was_underscore, false) || is_upper(c)) {
        size += 1;
      }
      size += 1;
    }
    return size;
  }

  using buffer_type = std::array<char, count()>;

  static consteval auto make_buffer() {
    constexpr auto sv = std::string_view {Name};

    buffer_type ret {};
    std::size_t count = 0;
    ret[count++] = to_lower(sv.front());

    bool prev_was_underscore = false;
    for (const auto c: sv.substr(1)) {
      if (c == '_') {
        prev_was_underscore = true;
        continue;
      }
      if (std::exchange(prev_was_underscore, false) || is_upper(c)) {
        ret[count++] = '-';
      }
      ret[count++] = to_lower(c);
    }
    return ret;
  };

  static constexpr auto buffer = make_buffer();
};

template <auto Name>
struct to_upper_t : normalizer_t {
  using buffer_type = std::array<char, std::string_view {Name}.size()>;
  static constexpr auto make_buffer() {
    static constexpr std::string_view sv {Name};
    buffer_type ret {};
    for (std::size_t i = 0; i < sv.size(); ++i) {
      ret[i] = to_upper(sv[i]);
    }
    return ret;
  }

  static constexpr auto buffer = make_buffer();
};

template <auto Name>
struct underscore_t : normalizer_t {
  static consteval std::size_t count() {
    std::size_t size = 1;
    for (auto&& c: std::string_view {Name}.substr(1)) {
      size += is_upper(c) ? 2 : 1;
    }
    return size;
  }

  using buffer_type = std::array<char, count()>;

  static consteval auto make_buffer() {
    constexpr auto sv = std::string_view {Name};

    buffer_type ret {};
    std::size_t count = 0;
    ret[count++] = sv.front();
    for (auto&& c: sv.substr(1)) {
      if (is_upper(c)) {
        ret[count++] = '_';
        ret[count++] = c;
      } else {
        ret[count++] = c;
      }
    }
    return ret;
  }

  static constexpr auto buffer = make_buffer();
};

template <auto Name>
struct remove_prefix_t : normalizer_t {
  static consteval std::size_t prefix_size() {
    constexpr std::string_view sv {Name};
    if (sv.starts_with('_')) {
      return 1;
    }
    if (sv.starts_with("m_")) {
      return 2;
    }
    if (sv.size() >= 2 && sv[0] == 'm' && is_upper(sv[1])) {
      return 1;
    }
    return 0;
  }

  using buffer_type
    = std::array<char, std::string_view {Name}.size() - prefix_size()>;

  static consteval auto make_buffer() {
    buffer_type ret {};
    std::ranges::copy(std::views::drop(Name, prefix_size()), ret.begin());
    return ret;
  }

  static constexpr auto buffer = make_buffer();
};

template <auto Name>
struct upper_camel_t : normalizer_t {
  static_assert(!std::string_view {Name}.empty());
  static constexpr std::size_t count() {
    std::size_t size = 1;
    for (auto&& c: std::string_view {Name}.substr(1)) {
      if (c == '_') {
        continue;
      }
      ++size;
    }
    return size;
  }

  using buffer_type = std::array<char, count()>;

  static constexpr auto buffer = [] {
    constexpr auto sv = std::string_view {Name};

    buffer_type ret {};
    std::size_t count = 0;
    ret[count++] = to_upper(sv.front());
    bool capitalizeNext = false;
    for (auto&& c: sv.substr(1)) {
      if (c == '_') {
        capitalizeNext = true;
        continue;
      }

      if (std::exchange(capitalizeNext, false)) {
        ret[count++] = to_upper(c);
      } else {
        ret[count++] = c;
      }
    }
    return ret;
  }();
};

template <std::size_t N, std::size_t M>
struct concat_t {
  static constexpr std::size_t TotalSize = N + M - 2;
  concat_t() = delete;
  consteval concat_t(const char (&lhs)[N], const char (&rhs)[M]) {
    std::ranges::copy(lhs, lhs + N - 1, mBuf);
    std::ranges::copy(rhs, rhs + M - 1, mBuf + N - 1);
  }

  constexpr bool operator==(const std::string_view& other) const noexcept {
    return other == std::string_view {mBuf, TotalSize};
  }

  constexpr operator std::string_view() const noexcept {
    return std::string_view {mBuf, TotalSize};
  }

  constexpr std::string_view operator*() const noexcept {
    return *this;
  }

 private:
  char mBuf[TotalSize] {};
};

template <std::size_t N, std::size_t M>
consteval auto concat(const char (&lhs)[N], const char (&rhs)[M]) {
  return concat_t<N, M> {lhs, rhs};
}

}// namespace magic_args::detail::constexpr_strings

#endif
