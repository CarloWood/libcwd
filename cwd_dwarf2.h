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

class SymbolRangeInterface
{
 public:
  virtual ~SymbolRangeInterface() = default;

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
  virtual SymbolRangeInterface const* find_symbol(uintptr_t addr) const = 0;
};

// Return the registered ObjectFile that contains `addr` in one of its mapped
// PT_LOAD address ranges, or nullptr if no known object covers addr.
//
// The returned interface remains owned by dwarf2's object-file registry and
// is valid until the object is unregistered due to a call to dlclose.
extern ObjectFileInterface const* find_object_file(void const* addr LIBCWD_COMMA_TSD_PARAM);

} // namespace libcwd::dwarf2
