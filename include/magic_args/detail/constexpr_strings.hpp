// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP
#define MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP

#include <algorithm>
#include <ranges>
#include <utility>

namespace magic_args::detail::constexpr_strings {

template <std::size_t N>
struct constexpr_string {
  static constexpr std::size_t size = N - 1;
  std::array<char, size> value;

  consteval constexpr_string(const char (&str)[N]) {
    std::ranges::copy_n(str, size, value.begin());
  }
};

template <constexpr_string T>
consteval auto operator""_constexpr() {
  return T.value;
}

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

struct transformer_t {
  template <class Self>
    requires requires { Self::value; }
  constexpr operator std::string_view(this const Self&) noexcept {
    return std::string_view {Self::value};
  }

  template <std::equality_comparable_with<std::string_view> T, class Self>
    requires requires { Self::value; }
  constexpr bool operator==(this const Self&, T&& other) noexcept {
    return std::string_view {Self::value} == std::forward<T>(other);
  }
};

template <auto TData, auto TPrefix>
struct remove_prefix_t : transformer_t {
  static constexpr std::string_view input {TData};
  static constexpr std::string_view prefix {TPrefix};
  static constexpr bool matches = input.starts_with(prefix);
  static constexpr auto size
    = matches ? (input.size() - prefix.size()) : input.size();

  static constexpr auto value = [] {
    auto output = input;
    if constexpr (matches) {
      output.remove_prefix(prefix.size());
    }
    std::array<char, size> ret {};
    std::ranges::copy(output, ret.begin());
    return ret;
  }();
};

template <auto TData, auto TSuffix>
struct remove_suffix_t : transformer_t {
  static constexpr std::string_view input {TData};
  static constexpr std::string_view suffix {TSuffix};
  static constexpr bool matches = input.ends_with(suffix);
  static constexpr auto size
    = matches ? (input.size() - suffix.size()) : input.size();

  static constexpr auto value = [] {
    auto output = input;
    if constexpr (matches) {
      output.remove_suffix(suffix.size());
    }
    std::array<char, size> ret {};
    std::ranges::copy(output, ret.begin());
    return ret;
  }();
};

template <template <auto, auto> class T, auto TData, auto TFirst, auto... TRest>
struct fold_left_t : transformer_t {
  static constexpr auto value = [] {
    if constexpr (sizeof...(TRest) == 0) {
      return T<TData, TFirst>::value;
    } else {
      return fold_left_t<T, T<TData, TFirst>::value, TRest...>::value;
    }
  }();
};

template <auto TData, auto TFirst, auto... TRest>
using remove_prefixes_t = fold_left_t<remove_prefix_t, TData, TFirst, TRest...>;
template <auto TData, auto TFirst, auto... TRest>
using remove_suffixes_t = fold_left_t<remove_suffix_t, TData, TFirst, TRest...>;
template <auto TData, auto TFirst, auto... TRest>
using remove_prefixes_or_suffixes_t = remove_prefixes_t<
  remove_suffixes_t<TData, TFirst, TRest...>::value,
  TFirst,
  TRest...>;

template <auto T>
struct remove_template_args_t : transformer_t {
  static constexpr auto input = std::string_view {T};
  static constexpr auto size = [] {
    constexpr auto pos = input.find('<');
    if (pos == std::string_view::npos) {
      return input.size();
    }
    return pos;
  }();

  static constexpr auto value = [] {
    std::array<char, size> ret;
    std::ranges::copy(input.substr(0, size), ret.begin());
    return ret;
  }();
};

template <auto Name>
struct hyphenate_t : transformer_t {
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

  using value_type = std::array<char, count()>;

  static consteval auto make_value() {
    constexpr auto sv = std::string_view {Name};

    value_type ret {};
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

  static constexpr auto value = make_value();
};

template <auto Name>
struct to_upper_t : transformer_t {
  using value_type = std::array<char, std::string_view {Name}.size()>;
  static constexpr auto make_value() {
    static constexpr std::string_view sv {Name};
    value_type ret {};
    for (std::size_t i = 0; i < sv.size(); ++i) {
      ret[i] = to_upper(sv[i]);
    }
    return ret;
  }

  static constexpr auto value = make_value();
};

template <auto Name>
struct underscore_t : transformer_t {
  static consteval std::size_t count() {
    std::size_t size = 1;
    for (auto&& c: std::string_view {Name}.substr(1)) {
      size += is_upper(c) ? 2 : 1;
    }
    return size;
  }

  using value_type = std::array<char, count()>;

  static consteval auto make_value() {
    constexpr auto sv = std::string_view {Name};

    value_type ret {};
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

  static constexpr auto value = make_value();
};

template <auto Name>
struct remove_field_prefix_t : transformer_t {
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

  using value_type
    = std::array<char, std::string_view {Name}.size() - prefix_size()>;

  static consteval auto make_value() {
    value_type ret {};
    std::ranges::copy(std::views::drop(Name, prefix_size()), ret.begin());
    return ret;
  }

  static constexpr auto value = make_value();
};

template <auto Name>
struct upper_camel_t : transformer_t {
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

  using value_type = std::array<char, count()>;

  static constexpr auto value = [] {
    constexpr auto sv = std::string_view {Name};

    value_type ret {};
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
  static constexpr auto value_size = N + 1;// trailing null

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
  static constexpr auto value_size = N;

  static constexpr auto lhs_subrange(auto&& lhs) {
    return std::views::all(std::forward<decltype(lhs)>(lhs));
  }

  static constexpr auto rhs_subrange(auto&& rhs) {
    return std::views::all(std::forward<decltype(rhs)>(rhs));
  }

  static constexpr auto dereference(const auto& buf) {
    return buf;
  }
};

template <std::size_t N, std::size_t M, class Traits = concat_c_string_traits>
struct concat_t {
  using element_type = Traits::element_type;

  static constexpr std::size_t size
    = Traits::lhs_subrange(std::array<element_type, N> {}).size()
    + Traits::rhs_subrange(std::array<element_type, M> {}).size();

  using buffer_type
    = std::array<element_type, Traits::template value_size<size>>;

  concat_t() = delete;

  template <class LHS, class RHS>
  consteval concat_t(LHS&& lhs, RHS&& rhs, std::type_identity<Traits> = {}) {
    const auto lhsSlice
      = Traits::lhs_subrange(std::views::all(std::forward<LHS>(lhs)));
    const auto rhsSlice
      = Traits::rhs_subrange(std::views::all(std::forward<RHS>(rhs)));
    const auto afterLhs = std::ranges::copy(lhsSlice, mBuf.begin()).out;
    std::ranges::copy(rhsSlice, afterLhs);
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

template <class T>
struct c_or_cpp_array_extent_t;

template <class E, std::size_t N>
struct c_or_cpp_array_extent_t<std::array<E, N>>
  : std::integral_constant<std::size_t, N> {};

template <class E, std::size_t N>
struct c_or_cpp_array_extent_t<E[N]> : std::integral_constant<std::size_t, N> {
};

template <class T>
constexpr auto c_or_cpp_array_extent_v
  = c_or_cpp_array_extent_t<std::remove_cvref_t<T>>::value;

template <class T, class U>
concat_t(T&&, U&&)
  -> concat_t<c_or_cpp_array_extent_v<T>, c_or_cpp_array_extent_v<U>>;
template <class T, class U, class V>
concat_t(T&&, U&&, V&&) -> concat_t<
  c_or_cpp_array_extent_v<T>,
  c_or_cpp_array_extent_v<U>,
  typename std::remove_cvref_t<V>::type>;

template <
  class Traits = concat_c_string_traits,
  class First,
  class Second,
  class... Rest>
consteval auto concat(First&& first, Second&& second, Rest&&... rest) {
  const auto lhs = concat_t {
    std::forward<First>(first),
    std::forward<Second>(second),
    std::type_identity<Traits> {},
  };
  if constexpr (sizeof...(Rest) == 0) {
    return lhs;
  } else {
    return concat<Traits>(lhs.get_buffer(), std::forward<Rest>(rest)...);
  }
}

}// namespace magic_args::detail::constexpr_strings

#endif
