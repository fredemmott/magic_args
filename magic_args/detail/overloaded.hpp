// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

namespace magic_args::detail {
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
}// namespace magic_args::detail