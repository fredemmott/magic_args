// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {
template <std::size_t N>
struct string_literal {
  static constexpr std::size_t size = N - 1;
  char buffer[size];
  consteval string_literal(const char (&str)[N]) {
    std::ranges::copy_n(str, size, buffer);
  }
};

template <string_literal T>
consteval auto operator""_array() {
  return std::to_array(T.buffer);
}
}// namespace

using namespace magic_args::detail::constexpr_strings;

TEST_CASE("concat strings") {
  STATIC_CHECK(concat("foo", "bar") == "foobar");
  STATIC_CHECK(concat("", "bar") == "bar");
  STATIC_CHECK(concat("foo", "") == "foo");
}

TEST_CASE("concat byte arrays") {
  using T = concat_byte_array_traits;
  STATIC_CHECK(concat<T>("foo", "bar") == "foo\0bar\0"_array);
  STATIC_CHECK(concat<T>("", "bar") == "\0bar\0"_array);
  STATIC_CHECK(concat<T>("foo", "") == "foo\0\0"_array);
}

TEST_CASE("hyphenate_t") {
  STATIC_CHECK(hyphenate_t<"f"_array> {} == "f");
  STATIC_CHECK(hyphenate_t<"F"_array> {} == "f");
  STATIC_CHECK(hyphenate_t<"Foo"_array> {} == "foo");
  STATIC_CHECK(hyphenate_t<"FooBar"_array> {} == "foo-bar");
  STATIC_CHECK(hyphenate_t<"Foo_bar"_array> {} == "foo-bar");
  STATIC_CHECK(hyphenate_t<"Foo_Bar"_array> {} == "foo-bar");
}

TEST_CASE("remove_prefix_t") {
  STATIC_CHECK(remove_prefix_t<"f"_array> {} == "f");
  STATIC_CHECK(remove_prefix_t<"m"_array> {} == "m");
  STATIC_CHECK(remove_prefix_t<"foo"_array> {} == "foo");
  STATIC_CHECK(remove_prefix_t<"my_thing"_array> {} == "my_thing");
  STATIC_CHECK(remove_prefix_t<"mFoo"_array> {} == "Foo");
  STATIC_CHECK(remove_prefix_t<"m_foo"_array> {} == "foo");
  STATIC_CHECK(remove_prefix_t<"_foo"_array> {} == "foo");
}

TEST_CASE("upper_camel_t") {
  STATIC_CHECK(upper_camel_t<"foo"_array> {} == "Foo");
  STATIC_CHECK(upper_camel_t<"fooBar"_array> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_bar"_array> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_Bar"_array> {} == "FooBar");
}