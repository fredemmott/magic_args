---
layout: default
title: Name normalization
parent: Features
---

# Argument name normalization

*magic_args* automatically normalizes C++ field names to match the expected style of the command-line interface; for example:

| C++ Field Name | GNU Style (`--foo-bar`) | PowerShell Style (`-FooBar`) |
|----------------|-------------------------|------------------------------|
| `mEmUpperCamel` | `--em-upper-camel` | `-EmUpperCamel` |
| `m_EmUnderscoreUpperCamel` | `--em-underscore-upper-camel` | `-EmUnderscoreUpperCamel` |
| `_UnderscoreUpperCamel` | `--underscore-upper-camel` | `-UnderscoreUpperCamel` |
| `_underscoreLowerCamel` | `--underscore-lower-camel` | `-UnderscoreLowerCamel` |
| `UpperCamel` | `--upper-camel` | `-UpperCamel` |
| `lowerCamel` | `--lower-camel` | `-LowerCamel` |
| `m_em_snake_case` | `--em-snake-case` | `-EmSnakeCase` |
| `snake_case` | `--snake-case` | `-SnakeCase` |
| `member_starts_with_m_but_is_not_m_prefix` | `--member-starts-with-m-but-is-not-m-prefix` | `-MemberStartsWithMButIsNotMPrefix` |

## Overriding for specific fields

Instead of using raw value types for your arguments, you can use the *magic_args* wrapper types, like `magic_args::option`; these have a `mName` member that can be used to override the default type:

```c++
struct MyArgs {
    magic_args::option<std::string> mFoo {
        .mName = "other", // --other=VALUE
        .mShortName = "o", // -o VALUE
    };
};

MAGIC_ARGS_MAIN(const MyArgs& args) {
    std::println("Value: {}", *args.mFoo);
}
```

Short names are not inferred and must be specified.

{: .note }
`magic_args::option<T>` defaults to value `T {}`; you can override the default by specifying `.mValue`. If you want a distinct 'unset' state, use `magic_args::option<std::optional<T>>`.

## Disabling normalization

You can entirely disable normalization by using the `verbatim_names` helper, e.g.:

```c++
struct MyArgs {
    // Alternatively, use `powershell_style_parsing_traits` to keep using a
    // single hyphen instead of two hyphens
    using parsing_traits =
        magic_args::verbatim_names<magic_args::gnu_style_parsing_traits>;
    
    std::string mFoo; // Argument is `--mFoo`, not `--foo`
};
```

## Custom normalization

You can implement the `magic_args::parsing_traits` concept yourself, potentially extending the predefined parsing traits.

`parsing_traits` classes must implement two static methods for normalization:

- `static constexpr auto template<auto Name> normalize_option_name();`
- `static constexpr auto template<auto Name> normalize_positional_argument_name();`

These must:
- be compile-time evaluated - all argument normalization is at compile-time
- accept `std::array<char, N>`
- return something where `std::string_view { return_value }` is valid