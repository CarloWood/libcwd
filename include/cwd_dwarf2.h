#pragma once

#ifndef CWD_DWARF2_H
#define CWD_DWARF2_H

#include "libcwd/config.h"
#include "threadsafe/threadsafe.h"
#include "threadsafe/AIReadWriteMutex.h"

#if CWDEBUG_LOCATION

#include "libcwd/ObjectFileName.h"
#include <list>

namespace libcwd::dwarf2 {

// Forward declare private class, defined in dwarf2.cc.
class ObjectFile;

// Base class of ObjectFile.
class ObjectFileBase
{
 protected:
  using object_files_t = threadsafe::Unlocked<std::list<ObjectFile*>, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  static object_files_t s_object_files_;        // Read-write lock protected list with all loaded object files.

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
