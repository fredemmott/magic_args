// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

// This test mostly exists so we can test basic changes to the demanglers
// without needing to switch environments constantly.
//
// If the mangled names change, that's fine, just update the test data

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

struct foo {
  std::string bar;
};

namespace MyNS {
struct bar {};
}// namespace MyNS

enum class MyEnum {
  Foo,
  Bar,
};

struct demangled_and_mangled_name {
  const std::string_view demangled {};

  const std::string_view mangled_apple_clang {};
  const std::string_view mangled_clang {};
  const std::string_view mangled_gcc {};
  const std::string_view mangled_msvc {};

  constexpr std::string_view mangled() const noexcept {
#if defined(__apple_build_version__)
    return mangled_apple_clang;
#elif defined(__clang__)
    return mangled_clang;
#elif defined(_MSC_VER)
    return mangled_msvc;
#elif defined(__GNUC__)
    return mangled_gcc;
#else
    static_assert(false, "Add the current compiler here");
#endif
  }
};

template <template <auto> class TDemangler, auto DataFn>
consteval auto demangle_with() {
  static constexpr auto mangled = [] {
    constexpr std::string_view sv = DataFn();
    std::array<char, sv.size()> arr {};
    std::ranges::copy(sv, arr.begin());
    return arr;
  }();
  return std::string_view {TDemangler<mangled>::value};
}

template <class T>
consteval auto mangled_type_name() {
  return magic_args::detail::mangled_name<T>();
}

constexpr auto type_test_data(std::type_identity<foo>) {
  return demangled_and_mangled_name {
    .demangled = "foo",
    .mangled_apple_clang
    = R"(auto magic_args::detail::mangled_name_c_str(void) [T = foo])",
    .mangled_clang
    = R"(auto __cdecl magic_args::detail::mangled_name_c_str(void) [T = foo])",
    .mangled_gcc
    = R"(constexpr auto magic_args::detail::mangled_name_c_str() [with T = foo])",
    .mangled_msvc
    = R"(auto __cdecl magic_args::detail::mangled_name_c_str<struct foo>(void))",
  };
}

constexpr auto type_test_data(std::type_identity<MyNS::bar>) {
  return demangled_and_mangled_name {
    .demangled = "MyNS::bar",
    .mangled_apple_clang
    = R"(auto magic_args::detail::mangled_name_c_str(void) [T = MyNS::bar])",
    .mangled_clang
    = R"(auto __cdecl magic_args::detail::mangled_name_c_str(void) [T = MyNS::bar])",
    .mangled_gcc
    = R"(constexpr auto magic_args::detail::mangled_name_c_str() [with T = MyNS::bar])",
    .mangled_msvc
    = R"(auto __cdecl magic_args::detail::mangled_name_c_str<struct MyNS::bar>(void))",
  };
}

using MyAliasCommand = foo;

TEMPLATE_TEST_CASE(
  "current type demangler",
  "",
  foo,
  MyNS::bar,
  MyAliasCommand) {
  using namespace magic_args::detail;
  static constexpr auto expected
    = type_test_data(std::type_identity<TestType> {});
  static constexpr auto mangled = mangled_name<TestType>();
  static constexpr auto demangled = demangle_type<mangled>();
  CHECK(std::string_view {mangled} == expected.mangled());
  CHECK(std::string_view {demangled} == expected.demangled);
}

TEMPLATE_TEST_CASE("all type demanglers", "", foo, MyNS::bar, MyAliasCommand) {
  using namespace magic_args::detail;

  static constexpr auto Data = type_test_data(std::type_identity<TestType> {});
  constexpr std::string_view Expected {Data.demangled};

  CHECK(
    demangle_with<
      clang_type_demangler_t,
      [] { return Data.mangled_apple_clang; }>()
    == Expected);
  CHECK(
    demangle_with<clang_type_demangler_t, [] { return Data.mangled_clang; }>()
    == Expected);
  CHECK(
    demangle_with<gcc_type_demangler_t, [] { return Data.mangled_gcc; }>()
    == Expected);
  CHECK(
    demangle_with<msvc_type_demangler_t, [] { return Data.mangled_msvc; }>()
    == Expected);
}

static constexpr auto ExampleField = demangled_and_mangled_name {
  .demangled = "bar",
  .mangled_apple_clang
  = R"(auto magic_args::detail::mangled_name_c_str() [T = apple_workaround_t<string>{&external.bar}])",
  .mangled_clang
  = R"(auto __cdecl magic_args::detail::mangled_name_c_str(void) [T = apple_workaround_t<basic_string<char>>{&external.bar}])",
  .mangled_gcc
  = R"(constexpr auto magic_args::detail::mangled_name_c_str() [with auto T = apple_workaround_t<std::__cxx11::basic_string<char> >{(& external<foo>.foo::bar)}])",
  .mangled_msvc
  = R"(auto __cdecl magic_args::detail::mangled_name_c_str<struct magic_args::detail::apple_workaround_t<class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > >{class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >*:&magic_args::detail::external<struct foo>->bar}>(void))",
};

TEST_CASE("mangled") {
  using namespace magic_args::detail;

  static constexpr std::string_view Mangled {mangled_name_by_index<foo, 0>};
  // Not using STATIC_CHECK() because we want to see the actual value
  CHECK(Mangled == ExampleField.mangled());
}

TEST_CASE("current demangler") {
  using namespace magic_args::detail;
  static constexpr std::string_view Demangled {member_name_by_index<foo, 0>};
  CHECK(Demangled == ExampleField.demangled);
}

TEST_CASE("all demanglers") {
  using namespace magic_args::detail;
  CHECK(
    std::string_view {demangle_with<
      clang_member_demangler_t,
      [] { return ExampleField.mangled_apple_clang; }>()}
    == ExampleField.demangled);
  CHECK(
    std::string_view {demangle_with<
      clang_member_demangler_t,
      [] { return ExampleField.mangled_clang; }>()}
    == ExampleField.demangled);
  CHECK(
    std::string_view {
      demangle_with<gcc_demangler_t, [] { return ExampleField.mangled_gcc; }>()}
    == ExampleField.demangled);
  CHECK(
    std::string_view {demangle_with<
      msvc_member_demangler_t,
      [] { return ExampleField.mangled_msvc; }>()}
    == ExampleField.demangled);
}