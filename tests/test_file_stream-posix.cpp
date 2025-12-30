// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <unistd.h>

#include "test_output.hpp"

void test_file_stream::reset() {
  this->wait();
  mData.clear();

  int pipefd[2];
  pipe(pipefd);
  mRead = fdopen(pipefd[0], "r");
  mWrite = fdopen(pipefd[1], "w");
  mFuture = std::async(std::launch::async, &test_file_stream::run, this);
}