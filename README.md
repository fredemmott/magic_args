# magic_args

*magic_args* is a C++23 header-only library for command-line argument handling. Its primary goal is ease of use, while
also aiming to be full-featured.

## Example

```c++
#include <magic_args/magic_args.hpp>

struct MyArgs {
  bool foo {false};
  std::string bar;
  int baz {0};
};

int main(int argc, char** argv) {
  // This gets you an
  // std::expected<MyArgs, magic_args::incomplete_parse_reason>
  const auto args = magic_args::parse<MyArgs>(argc, argv);
  
  if (!args.has_value()) {
    // This could be an actual error, e.g. invalid argument,
    // or something like `--help` or `--version`, which while not an error,
    // are an 'unexpected' outcome in the std::expected
    return magic_args::is_error(args.error()) ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  magic_args::dump(*args);
  return EXIT_SUCCESS;
}
```

## More information

See [the project website](https://fredemmott.github.io/magic_args).

## License

*magic_args* is [MIT-licensed](LICENSE).