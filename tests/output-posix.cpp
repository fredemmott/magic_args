// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <unistd.h>

#include "output.hpp"

Output::Output() {
  int pipefd[2];
  pipe(pipefd);
  mRead = fdopen(pipefd[0], "r");
  mWrite = fdopen(pipefd[1], "w");
  mFuture = std::async(std::launch::async, &Output::run, this);
}