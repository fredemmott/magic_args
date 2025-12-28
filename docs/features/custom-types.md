---
layout: default
title: Encodings
parent: Features
---

# Custom types

{: .no_toc }

*magic_args* automatically supports types which:

- satisfy the `std::formattable<char>` concept
- can be converted to a string by:
  - satisfying the `std::assignable_from<std::string_view>` concept
  - supporting `stream >> foo`
    - `std::ispanstream` will be used where available ([`__cpp_lib_spanstream` FTM][spanstream FTM])
    - otherwise, `std::stringstream` will be used; this mostly applies to Apple's variant of Clang

You can override these or add support for additional types by implementing these functions in the same namespace as the type:

```c++
auto to_argument_value(const YourType&)
  -> std::formattable<char> auto;

std::expected<void, magic_args::invalid_argument_value>
  from_argument_value(YourType& out, std::string_view in);
```

[spanstream FTM]: https://en.cppreference.com/w/cpp/experimental/feature_test.html#cpp_lib_spanstream