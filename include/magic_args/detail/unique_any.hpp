// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#ifndef MAGIC_ARGS_DETAIL_UNIQUE_ANY_HPP
#define MAGIC_ARGS_DETAIL_UNIQUE_ANY_HPP

#include <concepts>
#include <functional>
#include <utility>

namespace magic_args::detail {
/** like `unique_ptr`, but works with `void*` and non-pointer types.
 *
 * For example:
 * - `locale_t` is not guaranteed to be a pointer
 * - even when it is, it can be `void*` (e.g. on macos)
 *
 * A predicate is taken instead of an invalid value because some libraries
 * (e.g. iconv) use `(some_ptr) -1` as the sentinel value. Casting -1
 * to a pointer is never valid in constexpr, so we need this slightly
 * more verbose API.
 */
template <
  class T,
  std::invocable<T> auto TDeleter,
  std::predicate<T> auto TPredicate = std::identity {}>
struct unique_any {
  unique_any() = delete;
  unique_any(const unique_any&) = delete;
  unique_any& operator=(const unique_any&) = delete;

  unique_any(T value) : mValue(value) {
  }

  unique_any(unique_any&& other) noexcept {
    *this = std::move(other);
  }

  unique_any& operator=(unique_any&& other) noexcept {
    mMoved = std::exchange(other.mMoved, true);
    mValue = std::move(other.mValue);
    return *this;
  }

  ~unique_any() {
    if (*this) {
      std::invoke(TDeleter, mValue);
    }
  }

  template <class Self>
  [[nodiscard]]
  constexpr decltype(auto) get(this Self&& self) {
    if (self.mMoved) {
      throw std::logic_error("Can't access a moved value");
    }
    return std::forward_like<Self>(self.mValue);
  }

  template <class Self>
  constexpr decltype(auto) operator->(this Self&& self) {
    return std::forward_like<Self>(&self.mValue);
  }

  operator bool() const noexcept {
    return (!mMoved) && TPredicate(mValue);
  }

 private:
  bool mMoved {false};
  T mValue {};
};

}// namespace magic_args::detail
#endif
