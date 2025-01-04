// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "output.hpp"

Output::Output() {
}

Output::~Output() {
  wait();
}

void Output::run() {
  char buffer[1024];
  while (true) {
    const auto count = fread(buffer, 1, std::size(buffer), mRead);
    if (!count) {
      return;
    }
    mData.append(buffer, count);
  }
}

void Output::wait() {
  if (!mWrite) {
    return;
  }
  fclose(mWrite);
  mWrite = {};

  mFuture.wait();
  fclose(mRead);
  mRead = {};
}