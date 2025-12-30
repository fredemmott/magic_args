// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdio>
#include <future>
#include <string>

#ifdef TEST_SINGLE_HEADER
#include <magic_args/magic_args.hpp>
#else
#include <magic_args/console_output.hpp>
#endif

using test_output = magic_args::capture_console_output;

class test_file_stream {
 public:
  test_file_stream();
  ~test_file_stream();

  const std::string& get() {
    wait();
    return mData;
  }

  [[nodiscard]]
  bool empty() {
    return get().empty();
  }

  void reset();
  std::FILE* output_file() const noexcept {
    return mWrite;
  }

 private:
  std::future<void> mFuture;
  FILE* mRead {nullptr};
  FILE* mWrite {nullptr};
  std::string mData;

  void wait();
  void run();
};

class test_file_output {
  test_file_stream mOutStream;
  test_file_stream mErrorStream;

 public:
  magic_args::print_text_sink out {mOutStream.output_file()};
  magic_args::print_text_sink error {mErrorStream.output_file()};

  test_file_output& reset() {
    mOutStream.reset();
    mErrorStream.reset();
    out.reset(mOutStream.output_file());
    error.reset(mErrorStream.output_file());
    return *this;
  }

  [[nodiscard]]
  const std::string& out_str() {
    return mOutStream.get();
  }

  [[nodiscard]]
  const std::string& error_str() {
    return mErrorStream.get();
  }

  [[nodiscard]]
  bool empty() noexcept {
    return out_str().empty() && error_str().empty();
  }
};
static_assert(magic_args::console_output<test_file_output>);