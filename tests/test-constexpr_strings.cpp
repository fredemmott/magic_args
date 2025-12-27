// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace magic_args::detail::constexpr_strings;

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
  STATIC_CHECK(upper_camel_t<"foo"_constexpr> {} == "Foo");
  STATIC_CHECK(upper_camel_t<"fooBar"_constexpr> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_bar"_constexpr> {} == "FooBar");
  STATIC_CHECK(upper_camel_t<"foo_Bar"_constexpr> {} == "FooBar");
}
template <auto TData>
using RemoveFooBarPrefixes
  = remove_prefixes_t<TData, "Foo"_constexpr, "Bar"_constexpr>;

TEST_CASE("remove_prefixes_t") {
  STATIC_CHECK(RemoveFooBarPrefixes<"foo"_constexpr> {} == "foo");
  STATIC_CHECK(RemoveFooBarPrefixes<""_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarPrefixes<"Foo"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarPrefixes<"Bar"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarPrefixes<"FooBar"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarPrefixes<"FooBarBaz"_constexpr> {} == "Baz");
  // Test ordering: doesn't start with 'Foo' until after we've applied 'Bar'
  STATIC_CHECK(RemoveFooBarPrefixes<"BarFoo"_constexpr> {} == "Foo");
  // Suffix, not prefix
  STATIC_CHECK(RemoveFooBarPrefixes<"HerpFoo"_constexpr> {} == "HerpFoo");
}

template <auto TData>
using RemoveFooBarSuffixes
  = remove_suffixes_t<TData, "Foo"_constexpr, "Bar"_constexpr>;

TEST_CASE("remove_suffixes_t") {
  STATIC_CHECK(RemoveFooBarSuffixes<"foo"_constexpr> {} == "foo");
  STATIC_CHECK(RemoveFooBarSuffixes<""_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarSuffixes<"Foo"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarSuffixes<"Bar"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarSuffixes<"BarFoo"_constexpr> {} == "");
  STATIC_CHECK(RemoveFooBarSuffixes<"BazBarFoo"_constexpr> {} == "Baz");
  // Test ordering: doesn't end with 'Foo' until after we've applied 'Bar'
  STATIC_CHECK(RemoveFooBarSuffixes<"FooBar"_constexpr> {} == "Foo");
  // Prefix, not suffix
  STATIC_CHECK(RemoveFooBarSuffixes<"FooHerp"_constexpr> {} == "FooHerp");
}

template <auto TData>
using RemovePrefixesOrSuffixes
  = remove_prefixes_or_suffixes_t<TData, "Foo"_constexpr, "Bar"_constexpr>;

TEST_CASE("remove_prefixes_or_suffixes_t") {
  STATIC_CHECK(RemovePrefixesOrSuffixes<"FooHerpBar"_constexpr> {} == "Herp");
}
