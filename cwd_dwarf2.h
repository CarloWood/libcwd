#pragma once

#include "libcwd/private_struct_TSD.h"

namespace libcwd {
namespace dwarf2 {

// Ensure that dwarf2's object-file registry has been populated before an
// address lookup tries to consult it.
//
// Returns false only when libcwd initialization recurses before the registry
// could be populated. Repeated successful calls are cheap and leave the existing
// registry untouched.
extern bool ensure_initialization(LIBCWD_TSD_PARAM);

} // namespace dwarf2
} // namespace libcwd
