// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace magic_args::detail::constexpr_strings;

template <class T, std::convertible_to<std::string_view> U>
  requires requires(T t) { std::string_view {T::value}; }
constexpr bool operator==(const T& t, const U& u) {
  return std::string_view {t.value} == u;
}

TEST_CASE("concat strings") {
  STATIC_CHECK(concat("foo", "bar") == "foobar");
  STATIC_CHECK(concat("", "bar") == "bar");
  STATIC_CHECK(concat("foo", "") == "foo");
}

TEST_CASE("concat byte arrays") {
  using T = concat_byte_array_traits;
  STATIC_CHECK(concat<T>("foo", "bar") == "foo\0bar\0"_constexpr);
  STATIC_CHECK(concat<T>("", "bar") == "\0bar\0"_constexpr);
  STATIC_CHECK(concat<T>("foo", "") == "foo\0\0"_constexpr);
}

TEST_CASE("hyphenate_t") {
  STATIC_CHECK(hyphenate_t<"f"_constexpr> {} == "f");
  STATIC_CHECK(hyphenate_t<"F"_constexpr> {} == "f");
  STATIC_CHECK(hyphenate_t<"Foo"_constexpr> {} == "foo");
  STATIC_CHECK(hyphenate_t<"FooBar"_constexpr> {} == "foo-bar");
  STATIC_CHECK(hyphenate_t<"Foo_bar"_constexpr> {} == "foo-bar");
  STATIC_CHECK(hyphenate_t<"Foo_Bar"_constexpr> {} == "foo-bar");
}

TEST_CASE("remove_field_prefix_t") {
  STATIC_CHECK(remove_field_prefix_t<"f"_constexpr> {} == "f");
  STATIC_CHECK(remove_field_prefix_t<"m"_constexpr> {} == "m");
  STATIC_CHECK(remove_field_prefix_t<"foo"_constexpr> {} == "foo");
  STATIC_CHECK(remove_field_prefix_t<"my_thing"_constexpr> {} == "my_thing");
  STATIC_CHECK(remove_field_prefix_t<"mFoo"_constexpr> {} == "Foo");
  STATIC_CHECK(remove_field_prefix_t<"m_foo"_constexpr> {} == "foo");
  STATIC_CHECK(remove_field_prefix_t<"_foo"_constexpr> {} == "foo");
}

TEST_CASE("remove_prefix_t") {
  STATIC_CHECK(
    remove_prefix_t<"CommandFoo"_constexpr, "Command"_constexpr> {} == "Foo");
  STATIC_CHECK(
    remove_prefix_t<"Foo"_constexpr, "Command"_constexpr> {} == "Foo");
  STATIC_CHECK(remove_prefix_t<""_constexpr, "Command"_constexpr> {} == "");

  // Suffix, not prefix
  STATIC_CHECK(
    remove_prefix_t<"FooCommand"_constexpr, "Command"_constexpr> {}
    == "FooCommand");
}

TEST_CASE("remove_suffix_t") {
  STATIC_CHECK(
    remove_suffix_t<"FooCommand"_constexpr, "Command"_constexpr> {} == "Foo");
  STATIC_CHECK(
    remove_suffix_t<"Foo"_constexpr, "Command"_constexpr> {} == "Foo");
  STATIC_CHECK(remove_suffix_t<""_constexpr, "Command"_constexpr> {} == "");

  // Prefix, not suffix
  STATIC_CHECK(
    remove_suffix_t<"CommandFoo"_constexpr, "Command"_constexpr> {}
    == "CommandFoo");
}

TEST_CASE("upper_camel_t") {
  STATIC_CHECK(upper_camel_t<"foo"_array> {} == "Foo");
  STATIC_CHECK(upper_camel_t<"fooBar"_array> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_bar"_array> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_Bar"_array> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo"_constexpr> {} == "Foo");
  STATIC_CHECK(upper_camel_t<"fooBar"_constexpr> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_bar"_constexpr> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_Bar"_constexpr> {} == "FooBar");
}