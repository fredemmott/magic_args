// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP
#define MAGIC_ARGS_DETAIL_CONSTEXPR_STRINGS_HPP

#include <algorithm>
#include <ranges>
#include <utility>

namespace magic_args::detail::constexpr_strings {

template <std::size_t N>
struct literal_t final {
  static constexpr std::size_t size = N;
  std::array<char, size> value {};

  consteval literal_t() = default;

  consteval literal_t(const char (&str)[N + 1]) {
    std::ranges::copy_n(str, size, value.begin());
  }

  consteval literal_t(const std::array<char, N>& in) : value(in) {
  }

  consteval operator std::string_view() const noexcept {
    return std::string_view {value};
  }
};
template <std::size_t N>
literal_t(const char (&)[N]) -> literal_t<N - 1>;

template <literal_t T>
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

struct result_t {
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
struct remove_prefix_t : result_t {
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
struct remove_suffix_t : result_t {
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
struct fold_left_t : result_t {
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
struct remove_template_args_t : result_t {
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
struct hyphenate_t : result_t {
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
struct to_upper_t : result_t {
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
struct underscore_t : result_t {
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
struct remove_field_prefix_t : result_t {
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
struct upper_camel_t : result_t {
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

template <auto... Ts>
struct concat_t : result_t {
  static constexpr auto value = [] {
    constexpr auto size = (0 + ... + std::string_view {Ts}.size());
    std::array<char, size> ret {};
    auto out = ret.begin();
    (
      [&out](const auto input) {
        out = std::ranges::copy(std::string_view {input}, out).out;
      }(Ts),
      ...);
    return ret;
  }();
};

template <auto T, char V>
struct append_char_t : result_t {
  static constexpr auto value = concat_t<T, std::array {V}>::value;
};
template <auto T>
using append_null_t = append_char_t<T, '\0'>;

}// namespace magic_args::detail::constexpr_strings

#endif
