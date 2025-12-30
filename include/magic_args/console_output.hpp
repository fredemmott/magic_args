// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_OUTPUT_HPP
#define MAGIC_ARGS_OUTPUT_HPP

#include <concepts>
#include <format>
#include <print>

namespace magic_args::inline public_api {

template <class T>
concept text_sink = requires(T& v) {
  v.print("{}", 123);
  v.println("{}", 123);
};

template <class T>
concept console_output = requires(T v) {
  { v.out } -> text_sink;
  { v.error } -> text_sink;
};

class print_text_sink {
  std::FILE* mTarget {nullptr};

 public:
  print_text_sink() = delete;
  template <std::convertible_to<std::FILE*> T>
  print_text_sink(T&& target) : mTarget(std::forward<T>(target)) {
  }

  void reset(std::FILE* target) noexcept {
    mTarget = target;
  }

  template <class... Args>
  void print(std::format_string<Args...> fmt, Args&&... args) {
    std::print(mTarget, fmt, std::forward<Args>(args)...);
  }

  template <class... Args>
  void println(std::format_string<Args...> fmt, Args&&... args) {
    std::println(mTarget, fmt, std::forward<Args>(args)...);
  }
};
static_assert(text_sink<print_text_sink>);

struct print_console_output {
  print_text_sink out {stdout};
  print_text_sink error {stderr};
};
static_assert(console_output<print_console_output>);

template <std::output_iterator<char> T>
class format_to_text_sink {
  T mIterator {};

 public:
  format_to_text_sink() = delete;
  format_to_text_sink(const T& target) : mIterator(target) {
  }

  // It would be surprising that these would *copy* the iterator, then mutate
  // the copy, rather than advancing the original iterator
  format_to_text_sink(const format_to_text_sink&) = delete;
  format_to_text_sink& operator=(const format_to_text_sink&) = delete;

  template <class... Args>
  void print(std::format_string<Args...> fmt, Args&&... args) {
    mIterator
      = std::vformat_to(mIterator, fmt.get(), std::make_format_args(args...));
  }

  template <class... Args>
  void println(std::format_string<Args...> fmt, Args&&... args) {
    mIterator
      = std::vformat_to(mIterator, fmt.get(), std::make_format_args(args...));
    *mIterator++ = '\n';
  }
};

class capture_console_output {
  using iterator_type = std::back_insert_iterator<std::string>;
  using sink_type = format_to_text_sink<iterator_type>;

  std::string mOut;
  std::string mError;

 public:
  capture_console_output() = default;

  sink_type out {std::back_inserter(mOut)};
  sink_type error {std::back_inserter(mError)};

  [[nodiscard]]
  const std::string& out_str() const noexcept {
    return mOut;
  }
  [[nodiscard]]
  const std::string& error_str() const noexcept {
    return mError;
  }

  [[nodiscard]]
  bool empty() const noexcept {
    return mOut.empty() && mError.empty();
  }
};
}// namespace magic_args::inline public_api

#endif