// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_OVERLOADED_HPP
#define MAGIC_ARGS_DETAIL_OVERLOADED_HPP

namespace magic_args::detail {
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
}// namespace magic_args::detail

#endif