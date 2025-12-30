// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include <fcntl.h>
#include <io.h>

#include "test_output.hpp"

void test_file_stream::reset() {
  this->wait();
  mData.clear();

  HANDLE read {};
  HANDLE write {};

  CreatePipe(&read, &write, nullptr, 0);
  const auto readFd
    = _open_osfhandle(reinterpret_cast<intptr_t>(read), _O_RDONLY);
  const auto writeFd
    = _open_osfhandle(reinterpret_cast<intptr_t>(write), _O_APPEND);
  mRead = _fdopen(readFd, "r");
  mWrite = _fdopen(writeFd, "w");
  mFuture = std::async(std::launch::async, &test_file_stream::run, this);
}