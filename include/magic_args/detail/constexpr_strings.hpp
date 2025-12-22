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

// "foo" + "bar" should be length 6, not length 8
// Input has null after foo, null after bar, we're dropping both
struct concat_c_string_traits {
  using element_type = char;
  template <std::size_t N>
  static constexpr auto buffer_size = N + 1;// trailing null

  static constexpr auto lhs_subrange(const auto& lhs) {
    return std::views::take(lhs, std::ranges::size(lhs) - 1);
  }

  static constexpr auto rhs_subrange(const auto& rhs) {
    return lhs_subrange(rhs);
  }

  template <std::size_t N>
  static constexpr auto dereference(const std::array<char, N>& buf) {
    static_assert(N > 0, "Must at least have a trailing null");
    return std::string_view {buf.data(), N - 1};
  }
};

struct concat_byte_array_traits {
  using element_type = char;
  template <std::size_t N>
  static constexpr auto buffer_size = N;

  static constexpr auto lhs_subrange(const auto& lhs) {
    return lhs;
  }

  static constexpr auto rhs_subrange(const auto& rhs) {
    return rhs;
  }

  static constexpr auto dereference(const auto& buf) {
    return buf;
  }
};

template <std::size_t N, std::size_t M, class Traits = concat_c_string_traits>
struct concat_t {
  using element_type = Traits::element_type;
  template <std::size_t C>
  using array_type = std::array<element_type, C>;
  template <std::size_t C>
  using c_array_type = const char[C];

  static constexpr std::size_t size
    = Traits::lhs_subrange(array_type<N> {}).size()
    + Traits::rhs_subrange(array_type<M> {}).size();

  using buffer_type = array_type<Traits::template buffer_size<size>>;

  concat_t() = delete;
  consteval concat_t(const array_type<N>& lhs, const array_type<M>& rhs) {
    const auto lhsSlice = Traits::lhs_subrange(lhs);
    const auto rhsSlice = Traits::rhs_subrange(rhs);
    const auto afterLhs = std::ranges::copy(lhsSlice, mBuf.begin()).out;
    std::ranges::copy(rhsSlice, afterLhs);
  }

  consteval concat_t(const c_array_type<N>& lhs, const c_array_type<M>& rhs)
    : concat_t(std::to_array(lhs), std::to_array(rhs)) {
  }

  constexpr auto get() const noexcept {
    return Traits::dereference(mBuf);
  }

  template <class T>
    requires requires(concat_t v, T other) { v.get() == other; }
  constexpr bool operator==(const T& other) const noexcept {
    return get() == other;
  }

  constexpr auto operator*() const noexcept {
    return get();
  }

  constexpr auto get_buffer() const noexcept {
    return mBuf;
  }

 private:
  buffer_type mBuf {};
};

template <
  class Traits = concat_c_string_traits,
  std::size_t N,
  std::size_t M,
  class T = Traits::element_type>
consteval auto concat(const T (&lhs)[N], const T (&rhs)[M]) {
  return concat_t<N, M, Traits> {lhs, rhs};
}

template <
  class Traits = concat_c_string_traits,
  std::size_t N,
  std::size_t M,
  class T = Traits::element_type>
consteval auto concat(
  const std::array<T, N>& lhs,
  const std::array<T, M>& rhs) {
  return concat_t<N, M, Traits> {lhs, rhs};
}

}// namespace magic_args::detail::constexpr_strings

#endif
