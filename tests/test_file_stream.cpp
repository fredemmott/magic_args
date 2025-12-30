// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "test_output.hpp"

test_file_stream::test_file_stream() {
  this->reset();
}

test_file_stream::~test_file_stream() {
  wait();
}

void test_file_stream::run() {
  char buffer[1024];
  while (true) {
    const auto count = fread(buffer, 1, std::size(buffer), mRead);
    if (!count) {
      return;
    }
    mData.append(buffer, count);
  }
}

void test_file_stream::wait() {
  if (!mWrite) {
    return;
  }
  fclose(mWrite);
  mWrite = {};

  mFuture.wait();
  fclose(mRead);
  mRead = {};
}