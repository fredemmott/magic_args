// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <expected>
#include <filesystem>
#include <string_view>

#if __has_include(<magic_args/detail/unique_any.hpp>)
#include <magic_args/detail/unique_any.hpp>
#else
#include <magic_args/magic_args.hpp>
#endif

// Definitions for unique_any usage
using unique_fd = magic_args::detail::unique_any<int, [](int fd) {
  if (fd >= 0)
    close(fd);
}>;

struct elf_image {
  void* mData {};
  size_t mSize {};
  constexpr bool operator==(const elf_image&) const noexcept = default;
  constexpr operator bool() const noexcept {
    return mSize && mData && mData != MAP_FAILED;
  }
};

using unique_elf = magic_args::detail::unique_any<elf_image, [](elf_image img) {
  if (img) {
    munmap(img.mData, img.mSize);
  }
}>;

inline std::expected<unique_elf, const char*> open_library(
  const std::filesystem::path& path) {
  // Wrap the FD immediately
  const auto fd = unique_fd {open(path.c_str(), O_RDONLY)};
  if (!fd)
    return std::unexpected {"Could not open file"};

  struct stat st;
  if (fstat(fd.get(), &st) < 0)
    return std::unexpected {"fstat failed"};

  void* map = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd.get(), 0);
  if (map == MAP_FAILED)
    return std::unexpected {"mmap failed"};

  auto* ehdr = static_cast<Elf64_Ehdr*>(map);
  if (std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    munmap(map, st.st_size);
    return std::unexpected {"Not a valid ELF file"};
  }

  // fd is closed here automatically as it goes out of scope,
  // but the mapping remains valid.
  return unique_elf {elf_image {map, (size_t)st.st_size}};
}

inline size_t rva_to_offset(Elf64_Addr rva, Elf64_Ehdr* ehdr, uint8_t* map) {
  auto* phdr = reinterpret_cast<Elf64_Phdr*>(map + ehdr->e_phoff);
  for (int j = 0; j < ehdr->e_phnum; j++) {
    if (
      phdr[j].p_type == PT_LOAD && rva >= phdr[j].p_vaddr
      && rva < phdr[j].p_vaddr + phdr[j].p_memsz) {
      return (rva - phdr[j].p_vaddr) + phdr[j].p_offset;
    }
  }
  return 0;
}

enum class SymbolLookupError {
  TableNotFound,
  SymbolNotFound,
  AddressTranslationFailed,
};

template <>
struct std::formatter<SymbolLookupError> : std::formatter<std::string_view> {
  auto format(SymbolLookupError e, format_context& ctx) const {
    std::string_view name = "UnknownError";
    switch (e) {
      using enum SymbolLookupError;
      case TableNotFound:
        name = "TableNotFound";
        break;
      case SymbolNotFound:
        name = "SymbolNotFound";
        break;
      case AddressTranslationFailed:
        name = "AddressTranslationFailed";
        break;
    }
    return std::formatter<std::string_view>::format(name, ctx);
  }
};

inline std::expected<const char*, SymbolLookupError> get_data_pointer(
  const unique_elf& elf,
  const int table) {
  auto* map = static_cast<uint8_t*>(elf->mData);
  auto* ehdr = reinterpret_cast<Elf64_Ehdr*>(map);
  auto* shdr = reinterpret_cast<Elf64_Shdr*>(map + ehdr->e_shoff);

  Elf64_Shdr* symbolTable = nullptr;
  Elf64_Shdr* stringTable = nullptr;

  for (int i = 0; i < ehdr->e_shnum; i++) {
    if (shdr[i].sh_type == table) {
      symbolTable = &shdr[i];
      stringTable = &shdr[shdr[i].sh_link];
      break;
    }
  }

  using enum SymbolLookupError;

  if (!symbolTable) {
    return std::unexpected {TableNotFound};
  }

  auto* syms = reinterpret_cast<Elf64_Sym*>(map + symbolTable->sh_offset);
  auto* strs = reinterpret_cast<char*>(map + stringTable->sh_offset);
  const int count = symbolTable->sh_size / sizeof(Elf64_Sym);

  for (int i = 0; i < count; i++) {
    if (
      std::string_view(strs + syms[i].st_name)
      == "magic_args_subcommands_list") {
      const size_t offset = rva_to_offset(syms[i].st_value, ehdr, map);
      if (offset == 0)
        return std::unexpected {AddressTranslationFailed};
      return reinterpret_cast<const char*>(map + offset);
    }
  }

  return std::unexpected {SymbolNotFound};
}

inline std::expected<const char*, SymbolLookupError> get_data_pointer(
  const unique_elf& elf) {
  const auto dynamic = get_data_pointer(elf, SHT_DYNSYM);
  if (dynamic) {
    return dynamic;
  }
  using enum SymbolLookupError;
  switch (dynamic.error()) {
    case TableNotFound:
    case SymbolNotFound:
      break;
    case AddressTranslationFailed:
      return dynamic;
  }

  return get_data_pointer(elf, SHT_SYMTAB);
}