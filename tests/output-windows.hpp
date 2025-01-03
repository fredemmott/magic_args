// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdio>
#include <future>
#include <string>

class Output {
 public:
  Output();
  ~Output();

  std::string get() {
    wait();
    return mData;
  }

  [[nodiscard]]
  bool empty() {
    return get().empty();
  }

  operator FILE*() const noexcept {
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