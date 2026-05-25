#pragma once

#include "libcwd/ObjectFileName.h"
#include "libcwd/private_struct_TSD.h"
#include <cstdint>

namespace libcwd::dwarf2 {

// Ensure that dwarf2's object-file registry has been populated before an
// address lookup tries to consult it.
//
// Returns false only when libcwd initialization recurses before the registry
// could be populated. Repeated successful calls are cheap and leave the existing
// registry untouched.
extern bool ensure_initialization(LIBCWD_TSD_PARAM);

class SymbolInterface
{
 protected:
  uintptr_t start_addr_;
  uintptr_t end_addr_;

 public:
  //symbol_ct const* objfile_ct::find_symbol(symbol_ct const& search_key) const
  SymbolInterface(uintptr_t start_addr, uintptr_t end_addr) : start_addr_(start_addr), end_addr_(end_addr) { }
  virtual ~SymbolInterface() = default;

  // Accessors.
  uintptr_t start_addr() const { return start_addr_; }
  uintptr_t end_addr() const { return end_addr_; }

  virtual char const* name() const = 0;
  virtual bool lookup_file_line(uintptr_t addr, unsigned int* line_out, char const** filepath_out, uintptr_t lbase) const = 0;
};

class ObjectFileInterface
{
 protected:
  uintptr_t const lbase_;                       // The load address of this object file.

  ObjectFileInterface(uintptr_t lbase) : lbase_(lbase) { }

 public:
  uintptr_t get_lbase() const { return lbase_; }
  virtual ObjectFileName const& get_object_file() const = 0;
  virtual SymbolInterface const* find_symbol(uintptr_t addr) const = 0;
};

// Return the registered ObjectFile that contains `addr` in one of its mapped
// PT_LOAD address ranges, or nullptr if no known object covers addr.
//
// The returned interface remains owned by dwarf2's object-file registry and
// is valid until the object is unregistered due to a call to dlclose.
extern ObjectFileInterface const* find_object_file(void const* addr LIBCWD_COMMA_TSD_PARAM);

} // namespace libcwd::dwarf2
