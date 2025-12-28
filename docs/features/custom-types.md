---
layout: default
title: Encodings
parent: Features
---

# Custom types

{: .no_toc }

*magic_args* automatically supports types which:

- satisfy `std::formattable<char>`
- support `operator>>` from a `std::stringstream`

You can override these or add support for additional types by implementing these functions in the same namespace as the type:

```c++
auto to_argument_value(const YourType&)
  -> std::formattable<char> auto;

std::expected<void, magic_args::invalid_argument_value>
  from_argument_value(YourType& out, std::string_view in);
```