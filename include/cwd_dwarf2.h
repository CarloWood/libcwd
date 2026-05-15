#pragma once

#ifndef CWD_DWARF2_H
#define CWD_DWARF2_H

#include "libcwd/config.h"
#include "threadsafe/threadsafe.h"
#include "threadsafe/AIReadWriteMutex.h"

#if CWDEBUG_LOCATION

#include "libcwd/ObjectFileName.h"
#include <cstdint>
#include <map>

namespace libcwd::dwarf2 {

// Forward declare private class, defined in dwarf2.cc.
class PTLoadSegment;

// Base class of ObjectFile.
class ObjectFileBase
{
 protected:
  // Address index for all currently discovered loadable ELF segments. The map key is the segment's one-past-the-end address;
  // the pointed-to PTLoadSegment stores the matching start address, flags, and ObjectFile owner.
  using object_files_t = threadsafe::Unlocked<std::map<std::uintptr_t, PTLoadSegment const*>, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  static object_files_t s_object_files_;        // Read-write lock protected end-address index of loaded PT_LOAD segments.

  libcwd::ObjectFileName object_file_name_;     // Public facing data of this object file. Just contains the filename
                                                // and whether or not debug info was available for this object file or not.

 public:
  // Construct an ObjectFileBase for the given filepath.
  ObjectFileBase(char const* filepath) : object_file_name_(filepath) { }

  // Accessor.
  libcwd::ObjectFileName const& object_file_name() const { return object_file_name_; }
};

} // namespace libcwd::dwarf2

#endif // CWDEBUG_LOCATION
#endif // CWD_DWARF2_H
