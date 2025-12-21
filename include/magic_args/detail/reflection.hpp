// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_REFLECTION_HPP
#define MAGIC_ARGS_DETAIL_REFLECTION_HPP

#include <algorithm>
#include <source_location>
#include <string>
#include <tuple>
#include <utility>

namespace magic_args::detail {

struct any_t {
  template <class T>
  constexpr operator T() const {
    return T {};
  };
};

template <class T, class... Args>
consteval std::size_t count_members(Args&&... args) {
  if constexpr (requires { T {any_t {}, args...}; }) {
    return count_members<T>(any_t {}, args...);
  } else {
    return sizeof...(args);
  }
}

template <class T>
constexpr decltype(auto) tie_struct(T&& v) {
  constexpr auto count = count_members<std::decay_t<T>>();
  // Can get rid of this mess with C++26 P1061R10 - 'Structured bindings can
  // introduce a pack'
  if constexpr (count > 16) {
    static_assert(count <= 16, "Only 16 values are currently supported");
  } else if constexpr (count == 16) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16]
      = v;
    return std::tie(
      v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16);
  } else if constexpr (count == 15) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15]
      = v;
    return std::tie(
      v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
  } else if constexpr (count == 14) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14] = v;
    return std::tie(
      v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14);
  } else if constexpr (count == 13) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13);
  } else if constexpr (count == 12) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12);
  } else if constexpr (count == 11) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11);
  } else if constexpr (count == 10) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10);
  } else if constexpr (count == 9) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8, v9] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8, v9);
  } else if constexpr (count == 8) {
    auto& [v1, v2, v3, v4, v5, v6, v7, v8] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7, v8);
  } else if constexpr (count == 7) {
    auto& [v1, v2, v3, v4, v5, v6, v7] = v;
    return std::tie(v1, v2, v3, v4, v5, v6, v7);
  } else if constexpr (count == 6) {
    auto& [v1, v2, v3, v4, v5, v6] = v;
    return std::tie(v1, v2, v3, v4, v5, v6);
  } else if constexpr (count == 5) {
    auto& [v1, v2, v3, v4, v5] = v;
    return std::tie(v1, v2, v3, v4, v5);
  } else if constexpr (count == 4) {
    auto& [v1, v2, v3, v4] = v;
    return std::tie(v1, v2, v3, v4);
  } else if constexpr (count == 3) {
    auto& [v1, v2, v3] = v;
    return std::tie(v1, v2, v3);
  } else if constexpr (count == 2) {
    auto& [v1, v2] = v;
    return std::tie(v1, v2);
  } else if constexpr (count == 1) {
    auto& [v1] = v;
    return std::tie(v1);
  } else {
    static_assert(count == 0);
    return std::tuple {};
  }
}

template <auto T>
constexpr auto mangled_name_c_str() {
  return std::source_location::current().function_name();
}

template <auto T>
constexpr auto mangled_name() {
  // Don't inline the call to function_name(); if you do, VS2022 will truncate
  // the name
  constexpr auto data = mangled_name_c_str<T>();
  constexpr auto n = std::char_traits<char>::length(data);
  std::array<char, n> ret {};
  std::ranges::copy_n(data, n, ret.begin());
  return ret;
}

template <auto TName>
struct msvc_demangler_t {
  static constexpr auto view() {
    static_assert(!std::string_view {TName}.empty());
    // Mangled: const char *__cdecl mangled_name<&external<struct
    // Bar>->abc>(void)
    //
    // Or with apple_workaround_t:
    //
    // const char *--cdecl magic-args::detail::-reflection::mangled-name<struct
    // magic-args::detail::-reflection::apple-workaround-t<bool>{bool*:&magic-args::detail::-reflection::external<struct
    // -flags-only>->m-foo}>(void)
    constexpr std::string_view name {TName};
    constexpr auto begin = name.rfind("->") + 2;
    constexpr auto end = name.rfind('}');
    if constexpr (end != std::string_view::npos && end > begin) {
      return name.substr(begin, end - begin);
    } else {
      return name;
    }
  }

  static constexpr auto value = [] {
    std::array<char, view().size()> ret {};
    std::ranges::copy(view(), ret.begin());
    return ret;
  }();
};

template <auto TName>
struct clang_demangler_t {
  static constexpr auto view() {
    // Mangled: auto mangled_name() [T = &external.abc]
    //
    // ... or with apple_workaround_t {}:
    //
    // magic-args::detail::-reflection::mangled-name() [-t =
    // apple-workaround-t<bool>{&external.m-foo}]
    constexpr std::string_view name {TName};
    constexpr auto begin = name.rfind('.') + 1;
    constexpr auto end = name.rfind('}');
    if constexpr (end != std::string_view::npos && end > begin) {
      return name.substr(begin, end - begin);
    } else {
      return name;
    }
  }

  static constexpr auto value = [] {
    std::array<char, view().size()> ret {};
    std::ranges::copy(view(), ret.begin());
    return ret;
  }();
};

template <auto TName>
struct gcc_demangler_t {
  static constexpr auto view() {
    constexpr std::string_view name {TName};
    // auto magic-args::detail::-reflection::mangled-name() [with auto -t = (&
    // external<-flags-only>.-flags-only::m-foo)]
    //
    // ... or with apple_workaround_t {}:
    //
    // consteval auto magic-args::detail::-reflection::mangled-name() [with auto
    // -t = apple-workaround-t<bool>{(&
    // external<-flags-only>.-flags-only::m-foo)}]
    constexpr auto begin = name.rfind("::") + 2;
    constexpr auto end = name.rfind(')');
    if constexpr (end != std::string_view::npos && end > begin) {
      return name.substr(begin, end - begin);
    }
    return name;
  }

  static constexpr auto value = [] {
    std::array<char, view().size()> ret {};
    std::ranges::copy(view(), ret.begin());
    return ret;
  }();
};

template <auto Name>
consteval auto demangle() {
#if defined(__clang__)
  return clang_demangler_t<Name>::value;
#elif defined(_MSC_VER)
  return msvc_demangler_t<Name>::value;
#elif defined(__GNUC__)
  return gcc_demangler_t<Name>::value;
#else
#warning "Unsupported compiler detected"
  return Name;
#endif
}

template <auto T>
consteval auto demangled_name() {
  return demangle<mangled_name<T>()>();
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

// references to external variables have the handy habit of keeping the name of
// the variable when passing as a template value parameter
template <class T>
extern T external;

// Apple Clang 16.0 (XCode 16.2, latest as of 2025-01-03) won't allow
// a raw T* as a template parameter, but it will allow one of these
template <class T>
struct apple_workaround_t {
  T* ptr {nullptr};
};

template <class T, std::size_t N>
constexpr auto member_name = demangled_name<apple_workaround_t {
  &std::get<N>(tie_struct(external<T>))}>();

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}// namespace magic_args::detail

#endif
