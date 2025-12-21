// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <magic_args/magic_args.hpp>

#include <catch2/catch_test_macros.hpp>

struct foo {
  std::string bar;
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
  return TDemangler<mangled>::value;
}

// This test mostly exists so we can test basic changes to the demanglers
// without needing to switch environments constantly.
//
// If the mangled names change, that's fine, just update this
static constexpr auto TestData = demangled_and_mangled_name {
  .demangled = "bar",
  .mangled_apple_clang = R"(auto magic_args::detail::mangled_name_c_str() [T = apple_workaround_t<string>{&external.bar}])",
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
  CHECK(Mangled == TestData.mangled());
}

TEST_CASE("current demangler") {
  using namespace magic_args::detail;
  static constexpr std::string_view Demangled {member_name_by_index<foo, 0>};
  CHECK(Demangled == TestData.demangled);
}

TEST_CASE("all demanglers") {
  using namespace magic_args::detail;
  CHECK(
    std::string_view {
      demangle_with<clang_demangler_t, [] { return TestData.mangled_apple_clang; }>()}
    == TestData.demangled);
  CHECK(
    std::string_view {
      demangle_with<clang_demangler_t, [] { return TestData.mangled_clang; }>()}
    == TestData.demangled);
  CHECK(
    std::string_view {
      demangle_with<gcc_demangler_t, [] { return TestData.mangled_gcc; }>()}
    == TestData.demangled);
  CHECK(
    std::string_view {
      demangle_with<msvc_demangler_t, [] { return TestData.mangled_msvc; }>()}
    == TestData.demangled);
}