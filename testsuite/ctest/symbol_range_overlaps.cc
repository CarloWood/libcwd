// Verify deterministic resolution of overlapping `dwarf` function symbol ranges.
// The test exercises the private range insertion helper directly so it can cover ELF/DWARF
// precedence and same-provenance size ordering without needing synthetic object files.

#include "dwarf_symbol_ranges.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace libcwd;

// Return a non-null Dwarf_Die marker for constructing DWARF-backed ranges.
//
// The tests never query line information through this DIE, so only the addr field needs to distinguish it from an ELF-only range.
Dwarf_Die fake_dwarf_die()
{
  Dwarf_Die die{};
  die.addr = reinterpret_cast<void*>(0x1);
  return die;
}

// Check that ranges contains [start_addr, end_addr) with name, provenance and original symbol size.
//
// Returns false after printing a diagnostic that identifies the first mismatching field.
bool expect_range(dwarf::FunctionSymbolRanges const& ranges,
    uintptr_t start_addr, uintptr_t end_addr, char const* name, bool is_elf_symbol, uintptr_t symbol_size)
{
  auto const range = ranges.find(end_addr);
  if (range == ranges.end())
  {
    std::cerr << "missing range ending at " << end_addr << " for " << name << '\n';
    return false;
  }

  dwarf::SymbolRange const& symbol = range->second;
  if (symbol.start_addr() != start_addr || symbol.end_addr() != end_addr || std::strcmp(symbol.name(), name) != 0 ||
      symbol.is_elf_symbol() != is_elf_symbol || symbol.size() != symbol_size)
  {
    std::cerr << "range ending at " << end_addr << " was [" << symbol.start_addr() << ", " << symbol.end_addr() << ") "
        << symbol.name() << " elf=" << symbol.is_elf_symbol() << " size=" << symbol.size() << "; expected ["
        << start_addr << ", " << end_addr << ") " << name << " elf=" << is_elf_symbol << " size=" << symbol_size << '\n';
    return false;
  }
  return true;
}

struct ExpectedRange
{
  uintptr_t start_addr;
  uintptr_t end_addr;
  char const* name;
  bool is_elf_symbol;
  uintptr_t symbol_size;
};

// Check that ranges contains exactly the expected set of ranges for scenario.
//
// Each range is checked by its end-address map key and the total size check rejects unexpected leftovers or missing fragments.
bool expect_exact_ranges(char const* scenario, dwarf::FunctionSymbolRanges const& ranges, std::vector<ExpectedRange> const& expected)
{
  if (ranges.size() != expected.size())
  {
    std::cerr << scenario << ": produced " << ranges.size() << " ranges instead of " << expected.size() << '\n';
    return false;
  }

  for (ExpectedRange const& range : expected)
  {
    if (!expect_range(ranges, range.start_addr, range.end_addr, range.name, range.is_elf_symbol, range.symbol_size))
    {
      std::cerr << scenario << ": range check failed\n";
      return false;
    }
  }
  return true;
}

// Insert one of the oversized baseline ranges used by the complete-removal matrix.
//
// The original symbol size is deliberately much larger than the fragment so a same-kind new range from the scenario table wins every overlap by being smaller.
void insert_oversized_base_range(dwarf::FunctionSymbolRanges& ranges,
    uintptr_t start_addr, uintptr_t end_addr, char const* name, Dwarf_Die const* die)
{
  ranges.try_emplace(end_addr, start_addr, end_addr, 0, 100, name, die);
}

// Seed three same-kind ranges shaped like "****  ******      ****".
//
// The returned map uses an oversized original symbol extent for each fragment so complete removal can be tested for both ELF and DWARF additions.
void seed_three_base_ranges(dwarf::FunctionSymbolRanges& ranges, bool base_is_elf_symbol, Dwarf_Die const& dwarf_die)
{
  Dwarf_Die const* base_die = base_is_elf_symbol ? nullptr : &dwarf_die;
  insert_oversized_base_range(ranges, 0, 4, "base_left", base_die);
  insert_oversized_base_range(ranges, 6, 12, "base_middle", base_die);
  insert_oversized_base_range(ranges, 18, 22, "base_right", base_die);
}

// Build the expected result for adding a winning [new_start, new_end) range to the three baseline ranges.
//
// Existing ranges outside the new range survive unchanged, partially overlapped ranges are snipped, and fully covered ranges disappear.
std::vector<ExpectedRange> expected_after_winning_addition(uintptr_t new_start, uintptr_t new_end, bool base_is_elf_symbol, bool new_is_elf_symbol)
{
  struct BaseRange
  {
    uintptr_t start_addr;
    uintptr_t end_addr;
    char const* name;
  };

  BaseRange const base_ranges[] = {
    {0, 4, "base_left"},
    {6, 12, "base_middle"},
    {18, 22, "base_right"}
  };

  std::vector<ExpectedRange> expected;
  for (BaseRange const& base : base_ranges)
  {
    uintptr_t const overlap_start = std::max(base.start_addr, new_start);
    uintptr_t const overlap_end = std::min(base.end_addr, new_end);
    if (overlap_start >= overlap_end)
    {
      expected.push_back({base.start_addr, base.end_addr, base.name, base_is_elf_symbol, 100});
      continue;
    }

    if (base.start_addr < overlap_start)
      expected.push_back({base.start_addr, overlap_start, base.name, base_is_elf_symbol, 100});
    if (overlap_end < base.end_addr)
      expected.push_back({overlap_end, base.end_addr, base.name, base_is_elf_symbol, 100});
  }

  expected.push_back({new_start, new_end, "new_range", new_is_elf_symbol, new_end - new_start});
  return expected;
}

struct CompleteRemovalScenario
{
  char const* name;
  uintptr_t start_addr;
  uintptr_t end_addr;
};

// Verify complete and partial removals for the diagrammed overlap shapes.
//
// The same geometric cases are run with the requested base and new symbol kinds; callers only use combinations where the new symbol should win each overlap.
bool verify_complete_removal_shapes(bool base_is_elf_symbol, bool new_is_elf_symbol)
{
  CompleteRemovalScenario const scenarios[] = {
    {"**", 0, 2},
    {"****", 0, 4},
    {"*****", 0, 5},
    {" *****", 1, 6},
    {"   *****", 3, 8},
    {"      *****", 6, 11},
    {"       *****", 7, 12},
    {"      ******", 6, 12},
    {"   ****************", 3, 19},
    {"      ****************", 6, 22},
    {"         *************", 9, 22}
  };

  Dwarf_Die const dwarf_die = fake_dwarf_die();
  for (CompleteRemovalScenario const& scenario : scenarios)
  {
    dwarf::FunctionSymbolRanges ranges;
    seed_three_base_ranges(ranges, base_is_elf_symbol, dwarf_die);

    Dwarf_Die const* new_die = new_is_elf_symbol ? nullptr : &dwarf_die;
    dwarf::insert_function_symbol_range(ranges, scenario.start_addr, scenario.end_addr, "new_range", new_die);

    std::string const scenario_name = std::string("complete removal ") + scenario.name +
        " base=" + (base_is_elf_symbol ? "ELF" : "DWARF") + " new=" + (new_is_elf_symbol ? "ELF" : "DWARF");
    if (!expect_exact_ranges(scenario_name.c_str(), ranges,
        expected_after_winning_addition(scenario.start_addr, scenario.end_addr, base_is_elf_symbol, new_is_elf_symbol)))
      return false;
  }
  return true;
}

// Verify that a DWARF range replaces the overlapping part of an ELF range.
//
// The ELF range should be snipped into left and right leftovers that keep the original ELF symbol size.
bool verify_dwarf_beats_elf()
{
  dwarf::FunctionSymbolRanges ranges;
  Dwarf_Die const dwarf_die = fake_dwarf_die();

  dwarf::insert_function_symbol_range(ranges, 10, 50, "elf_big", nullptr);
  dwarf::insert_function_symbol_range(ranges, 20, 40, "dwarf_inner", &dwarf_die);

  if (ranges.size() != 3)
  {
    std::cerr << "DWARF over ELF produced " << ranges.size() << " ranges instead of 3\n";
    return false;
  }
  return expect_range(ranges, 10, 20, "elf_big", true, 40) &&
      expect_range(ranges, 20, 40, "dwarf_inner", false, 20) &&
      expect_range(ranges, 40, 50, "elf_big", true, 40);
}

// Verify that, for two DWARF ranges, the smaller original symbol owns the overlap.
//
// The larger existing range should be split around the new smaller range.
bool verify_smaller_same_kind_wins()
{
  dwarf::FunctionSymbolRanges ranges;
  Dwarf_Die const dwarf_die = fake_dwarf_die();

  dwarf::insert_function_symbol_range(ranges, 100, 200, "dwarf_big", &dwarf_die);
  dwarf::insert_function_symbol_range(ranges, 120, 160, "dwarf_small", &dwarf_die);

  if (ranges.size() != 3)
  {
    std::cerr << "smaller DWARF over larger DWARF produced " << ranges.size() << " ranges instead of 3\n";
    return false;
  }
  return expect_range(ranges, 100, 120, "dwarf_big", false, 100) &&
      expect_range(ranges, 120, 160, "dwarf_small", false, 40) &&
      expect_range(ranges, 160, 200, "dwarf_big", false, 100);
}

// Verify that a smaller existing DWARF range removes its overlap from a larger new DWARF range.
//
// The pending new range should be split into two inserted pieces that keep the larger original symbol size.
bool verify_existing_smaller_removes_pending_piece()
{
  dwarf::FunctionSymbolRanges ranges;
  Dwarf_Die const dwarf_die = fake_dwarf_die();

  dwarf::insert_function_symbol_range(ranges, 300, 330, "dwarf_small", &dwarf_die);
  dwarf::insert_function_symbol_range(ranges, 290, 350, "dwarf_big", &dwarf_die);

  if (ranges.size() != 3)
  {
    std::cerr << "larger DWARF around smaller DWARF produced " << ranges.size() << " ranges instead of 3\n";
    return false;
  }
  return expect_range(ranges, 290, 300, "dwarf_big", false, 60) &&
      expect_range(ranges, 300, 330, "dwarf_small", false, 30) &&
      expect_range(ranges, 330, 350, "dwarf_big", false, 60);
}

// Verify that an existing DWARF range blocks the overlapping part of a new ELF range.
//
// The non-overlapping ELF pieces should still be inserted on both sides of the DWARF range.
bool verify_existing_dwarf_blocks_elf_piece()
{
  dwarf::FunctionSymbolRanges ranges;
  Dwarf_Die const dwarf_die = fake_dwarf_die();

  dwarf::insert_function_symbol_range(ranges, 500, 550, "dwarf_middle", &dwarf_die);
  dwarf::insert_function_symbol_range(ranges, 480, 560, "elf_outer", nullptr);

  if (ranges.size() != 3)
  {
    std::cerr << "ELF around DWARF produced " << ranges.size() << " ranges instead of 3\n";
    return false;
  }
  return expect_range(ranges, 480, 500, "elf_outer", true, 80) &&
      expect_range(ranges, 500, 550, "dwarf_middle", false, 50) &&
      expect_range(ranges, 550, 560, "elf_outer", true, 80);
}

} // namespace

int main()
{
  return verify_dwarf_beats_elf() && verify_smaller_same_kind_wins() && verify_existing_smaller_removes_pending_piece() &&
      verify_existing_dwarf_blocks_elf_piece() && verify_complete_removal_shapes(true, true) &&
      verify_complete_removal_shapes(true, false) && verify_complete_removal_shapes(false, false) ? EXIT_SUCCESS : EXIT_FAILURE;
}
