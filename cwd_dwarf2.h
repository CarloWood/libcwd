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

class ObjectFileInterface
{
 protected:
  uintptr_t const lbase_;                       // The load address of this object file.

  ObjectFileInterface(uintptr_t lbase) : lbase_(lbase) { }

 public:
  uintptr_t get_lbase() const { return lbase_; }
};

} // namespace libcwd::dwarf2
