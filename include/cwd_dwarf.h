// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef CWD_DWARF_H
#define CWD_DWARF_H

#include "libcwd/ObjectFileName.h"
#include "libcwd/private/ThreadSpecificData.h"

#include <cstdint>

namespace libcwd::dwarf {

// Ensure that dwarf's object-file registry has been populated before an
// address lookup tries to consult it.
//
// Returns false only when libcwd initialization recurses before the registry
// could be populated. Repeated successful calls are cheap and leave the existing
// registry untouched.
extern bool ensure_initialization(LIBCWD_TSD_PARAM);

// Result of resolving a runtime address within one cached symbol range.
//
// physical_function_name is the enclosing object-code symbol that provided the
// address range. effective_function_name is an optional source-level override
// set only when inline-scope lookup learns that the PC belongs to an inlined
// function body.
// filepath points to libdw-owned storage and may be set even when known is false.
struct LocationLookupResult
{
  char const* physical_function_name{}; // Enclosing machine-code symbol.
  char const* effective_function_name{}; // Optional source-level override for reporting.
  char const* filepath{nullptr}; // Source path, or nullptr when no file is known.
  unsigned int line; // Source line, valid only when known is true.
  bool known{false}; // True when filepath and line describe a usable source location.

  // Return the function name selected for callers that do not need to inspect
  // why it was selected.
  //
  // This returns effective_function_name when inline-aware lookup has chosen
  // one, otherwise it falls back to physical_function_name.
  char const* function_name() const
  {
    return effective_function_name ? effective_function_name : physical_function_name;
  }
};

class SymbolRangeInterface
{
 public:
  virtual ~SymbolRangeInterface() = default;

  virtual char const* name() const = 0;

  // Resolve addr relative to this symbol range using lbase as the owning
  // object's load base.
  //
  // DWARF line lookups use addr - lbase. The returned pointers are borrowed
  // from storage owned by the DWARF reader or symbol cache and remain valid
  // under the same lifetime rules as name().
  virtual LocationLookupResult lookup_location(uintptr_t addr, uintptr_t lbase) const = 0;
};

class ObjectFileInterface
{
 protected:
  uintptr_t const lbase_; // The load address of this object file.

  ObjectFileInterface(uintptr_t lbase) : lbase_(lbase) { }

 public:
  uintptr_t get_lbase() const { return lbase_; }
  virtual ObjectFileName const& get_object_file() const = 0;
  virtual SymbolRangeInterface const* find_symbol(uintptr_t addr) const = 0;
};

// Return the registered ObjectFile that contains `addr` in one of its mapped
// PT_LOAD address ranges, or nullptr if no known object covers addr.
//
// The returned interface remains owned by dwarf's object-file registry and
// is valid until the object is unregistered due to a call to dlclose.
extern ObjectFileInterface const* find_object_file(void const* addr LIBCWD_COMMA_TSD_PARAM);

} // namespace libcwd::dwarf

#endif // CWD_DWARF_H
