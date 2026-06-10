// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef DWARF_SYMBOL_RANGES_H
#define DWARF_SYMBOL_RANGES_H

#include "cwd_sys.h"
#include "cwd_dwarf.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>
#include <elfutils/libdw.h>

namespace libcwd::dwarf {

// Represents one contiguous fragment of a function symbol range.
//
// A fragment keeps both its current [start_addr, end_addr) interval and the original unsnipped symbol interval so later
// overlap resolution can compare complete symbol sizes.
class SymbolRange : public SymbolRangeInterface
{
 private:
  uintptr_t start_addr_; // Current fragment start.
  uintptr_t end_addr_; // Current fragment end.

  uintptr_t symbol_start_addr_; // Original unsnipped symbol start.
  uintptr_t symbol_end_addr_; // Original unsnipped symbol end.

  char const* name_; // The linkage name of this function symbol.
  Dwarf_Die die_{}; // The function DIE of this function symbol, or empty when the symbol came only from ELF.

 public:
  // Construct a fragment [start_addr, end_addr) of the original [symbol_start_addr, symbol_end_addr) symbol with the
  // given name and optional DIE.
  //
  // The DIE pointer may be null for ELF-only symbols. The name pointer is borrowed and must remain stable for as long
  // as the range can be queried.
  SymbolRange(uintptr_t start_addr, uintptr_t end_addr, uintptr_t symbol_start_addr, uintptr_t symbol_end_addr,
              char const* name, Dwarf_Die const* die = nullptr) :
      start_addr_(start_addr),
      end_addr_(end_addr),
      symbol_start_addr_(symbol_start_addr),
      symbol_end_addr_(symbol_end_addr),
      name_(name)
  {
    LIBCWD_ASSERT(symbol_start_addr < symbol_end_addr);
    LIBCWD_ASSERT(start_addr < end_addr);
    // Must be a fragment of the original.
    LIBCWD_ASSERT(symbol_start_addr <= start_addr && end_addr <= symbol_end_addr);
    if (die)
      die_ = *die;
  }

  // Accessors.
  uintptr_t start_addr() const { return start_addr_; }
  uintptr_t end_addr() const { return end_addr_; }
  uintptr_t size() const { return symbol_end_addr_ - symbol_start_addr_; }
  char const* name() const override { return name_; }

  // A SymbolRange constructed with a null die will have originated from ELF symbol lookup.
  bool is_elf_symbol() const { return die_.addr == nullptr; }

  // Return a SymbolRange that describes the subrange [start_addr, end_addr) of this symbol.
  //
  // The new object preserves the original unsnipped symbol extent, name and DIE state.
  // This keeps winner comparisons based on the producing symbol size rather than the fragment size.
  SymbolRange fragment(uintptr_t start_addr, uintptr_t end_addr) const
  {
    return SymbolRange(start_addr, end_addr, symbol_start_addr_, symbol_end_addr_, name_,
                       is_elf_symbol() ? nullptr : &die_);
  }

  LocationLookupResult lookup_location(uintptr_t addr, uintptr_t lbase) const override;
};

using FunctionSymbolRanges = std::map<uintptr_t, SymbolRange>;

namespace detail {

using PendingSymbolPiece = std::pair<uintptr_t, uintptr_t>;

// Return true when the new symbol should own an overlap against existing_symbol.
//
// DWARF-backed ranges beat ELF-only ranges.
// When both ranges have the same provenance, the smaller original symbol size wins and ties keep the existing range
// stable.
inline bool new_symbol_range_wins(SymbolRange const& existing_symbol, bool new_is_elf_symbol, uintptr_t new_symbol_size)
{
  if (existing_symbol.is_elf_symbol() != new_is_elf_symbol)
    return existing_symbol.is_elf_symbol() && !new_is_elf_symbol;
  return new_symbol_size < existing_symbol.size();
}

// Remove [remove_start, remove_end) from pending_new_pieces.
//
// Pieces are kept as non-empty half-open intervals.
// A removal can erase a whole piece, trim either side, or split one piece into left and right leftovers.
inline void remove_pending_symbol_subrange(std::vector<PendingSymbolPiece>& pending_new_pieces, uintptr_t remove_start,
                                           uintptr_t remove_end)
{
  for (auto piece = pending_new_pieces.begin(); piece != pending_new_pieces.end();)
  {
    uintptr_t const piece_start = piece->first;
    uintptr_t const piece_end = piece->second;
    uintptr_t const overlap_start = std::max(piece_start, remove_start);
    uintptr_t const overlap_end = std::min(piece_end, remove_end);

    if (overlap_start >= overlap_end)
    {
      ++piece;
      continue;
    }

    if (overlap_start == piece_start && overlap_end == piece_end)
      piece = pending_new_pieces.erase(piece);
    else if (overlap_start == piece_start)
    {
      piece->first = overlap_end;
      ++piece;
    }
    else if (overlap_end == piece_end)
    {
      piece->second = overlap_start;
      ++piece;
    }
    else
    {
      piece->second = overlap_start;
      piece = pending_new_pieces.emplace(piece + 1, overlap_end, piece_end);
      ++piece;
    }
  }
}

} // namespace detail

// Insert the symbol [start_addr, end_addr) into function_symbols with the given name and optional DIE.
//
// Overlapping ranges are resolved per overlapping subrange. DWARF beats ELF, and otherwise the smaller original symbol
// size wins. Losing existing ranges are erased or snipped into leftovers while losing parts of the new range are not
// inserted. Returns true when at least one new fragment was inserted.
inline bool insert_function_symbol_range(FunctionSymbolRanges& function_symbols, uintptr_t start_addr,
                                         uintptr_t end_addr, char const* name, Dwarf_Die const* func_die)
{
  if (start_addr >= end_addr)
    return false;

  uintptr_t const new_symbol_size = end_addr - start_addr;
  bool const new_is_elf_symbol = func_die == nullptr;
  std::vector<detail::PendingSymbolPiece> pending_new_pieces{{start_addr, end_addr}};

  for (auto existing = function_symbols.upper_bound(start_addr); existing != function_symbols.end();)
  {
    SymbolRange const& existing_symbol = existing->second;
    uintptr_t const overlap_start = std::max(start_addr, existing_symbol.start_addr());
    uintptr_t const overlap_end = std::min(end_addr, existing_symbol.end_addr());
    if (overlap_start >= overlap_end)
    {
      ++existing;
      continue;
    }

    if (detail::new_symbol_range_wins(existing_symbol, new_is_elf_symbol, new_symbol_size))
    {
      SymbolRange const old_symbol = existing_symbol;
      existing = function_symbols.erase(existing);

      if (old_symbol.start_addr() < overlap_start)
        function_symbols.emplace(overlap_start, old_symbol.fragment(old_symbol.start_addr(), overlap_start));
      if (overlap_end < old_symbol.end_addr())
        function_symbols.emplace(old_symbol.end_addr(), old_symbol.fragment(overlap_end, old_symbol.end_addr()));
    }
    else
    {
      detail::remove_pending_symbol_subrange(pending_new_pieces, overlap_start, overlap_end);
      ++existing;
    }
  }

  bool inserted_new_fragment = false;
  for (auto const& [piece_start, piece_end] : pending_new_pieces)
  {
    auto const insert_result =
        function_symbols.try_emplace(piece_end, piece_start, piece_end, start_addr, end_addr, name, func_die);
    inserted_new_fragment = inserted_new_fragment || insert_result.second;
  }
  return inserted_new_fragment;
}

} // namespace libcwd::dwarf

#endif // DWARF_SYMBOL_RANGES_H
