// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#include "enumerate-subcommands_arguments.hpp"
#include "enumerate-subcommands_config.hpp"

#include <filesystem>

namespace fs = std::filesystem;

enum class [[nodiscard]] LinkState {
  Absent,
  Differs,
  UpToDate,
};

template <class Derived>
struct symlink_t {
  static constexpr auto kind = "symlink";

  static LinkState state(const fs::path& target, const fs::path& link) {
    switch (const auto stat = symlink_status(link); stat.type()) {
      case fs::file_type::not_found:
        return LinkState::Absent;
      case fs::file_type::symlink:
        if (read_symlink(link) != Derived::normalize_target(target, link)) {
          return LinkState::Differs;
        }
        return LinkState::UpToDate;
        break;
      default:
        return LinkState::Differs;
    }
  }

  static void create(const fs::path& target, const fs::path& link) {
    fs::create_symlink(Derived::normalize_target(target, link), link);
  }
};
struct absolute_symlink_t : symlink_t<absolute_symlink_t> {
  static fs::path normalize_target(const fs::path& target, const fs::path&) {
    return fs::canonical(target);
  }
};
struct relative_symlink_t : symlink_t<relative_symlink_t> {
  static fs::path normalize_target(
    const fs::path& target,
    const fs::path& link) {
    return fs::relative(target, link.parent_path());
  }
};
struct hardlink_t {
  static constexpr auto kind = "hardlink";
  static void create(const fs::path& target, const fs::path& link) {
    fs::create_hard_link(target, link);
  }
  static LinkState state(const fs::path& target, const fs::path& link) {
    switch (symlink_status(link).type()) {
      case fs::file_type::not_found:
        return LinkState::Absent;
      case fs::file_type::regular:
        return fs::equivalent(target, link) ? LinkState::UpToDate
                                            : LinkState::Differs;
      default:
        return LinkState::Differs;
    }
  }
};

template <class T>
concept link_kind = requires(const fs::path& p) {
  { T::kind } -> std::formattable<char>;
  T::create(p, p);
  { T::state(p, p) } -> magic_args::detail::same_as_ignoring_cvref<LinkState>;
};

template <link_kind TLink>
std::expected<void, int> create_link(
  const fs::path& target,
  const fs::path& root,
  const std::string_view name,
  const arguments& args) try {
  if (root.empty()) {
    return {};
  }

  const auto link
    = root / std::format("{}{}", name, BuildConfig::ExecutableSuffix);
  // Don't use fs::canonical because that follows symlinks
  const auto normalizedLink = fs::absolute(link.lexically_normal());
  switch (TLink::state(target, link)) {
    case LinkState::UpToDate:
      if (args.mOutputStyle == OutputStyle::CMakeInstall) {
        std::println("-- Up-to-date: {}", normalizedLink.string());
      }
      return {};
    case LinkState::Absent:
      break;
    case LinkState::Differs:
      if (args.mForce) {
        fs::remove(link);
      } else {
        std::println(
          stderr,
          "File already exists; use `--force` to overwrite: {}",
          normalizedLink.string());
        return std::unexpected {EXIT_FAILURE};
      }
      break;
  }

  if (args.mOutputStyle == OutputStyle::CMakeInstall) {
    std::println("-- Installing: {}", normalizedLink.string());
  }

  TLink::create(target, link);
  return {};
} catch (const fs::filesystem_error& e) {
  std::println(
    stderr,
    "Failed to create {} link `{}/{}{}`: {}",
    TLink::kind,
    root.string(),
    name,
    BuildConfig::ExecutableSuffix,
    e.what());
  return std::unexpected {EXIT_FAILURE};
}
