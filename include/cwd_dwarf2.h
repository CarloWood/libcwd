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
class ObjectFileData;

// Thread-safe storage wrapper for ObjectFile instances.
// Delayed symbol loading mutates per-object DWARF state while other threads can concurrently perform address lookups.
using object_file_data_t = threadsafe::Unlocked<ObjectFileData, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;

struct ObjectFile
{
  uintptr_t const lbase_;       // The load address of this object file.
  object_file_data_t* data_;    // This must be a pointer because ObjectFileData is incomplete at this point.

  ObjectFile(uintptr_t lbase, object_file_data_t* object_file_data) :
    lbase_(lbase), data_(object_file_data) { }
};

// class PTLoadSegment
//
// Represents one loadable runtime segment of an ObjectFile.  Instances are
// created while holding ObjectFileBase::s_object_files_' write lock during
// initialization and then treated as immutable; later address lookup can read
// the segment start/end/flags and object_file_.lbase_ without a per-segment lock.
class PTLoadSegment
{
 private:
  ObjectFile object_file_;
  uintptr_t start_addr_;
  uintptr_t end_addr_;
  uint32_t flags_;

 public:
  PTLoadSegment(uintptr_t lbase, object_file_data_t* object_file_data, uintptr_t start_addr, uintptr_t end_addr, uint32_t flags) :
    object_file_(lbase, object_file_data), start_addr_(start_addr), end_addr_(end_addr), flags_(flags) { }

  uintptr_t object_lbase() const { return object_file_.lbase_; }
  object_file_data_t* object_file_data() const { return object_file_.data_; }
  uintptr_t start_addr() const { return start_addr_; }
  uintptr_t end_addr() const { return end_addr_; }
  uint32_t flags() const { return flags_; }

  friend std::ostream& operator<<(std::ostream& os, PTLoadSegment const& segment)
  {
    os << '[' << std::hex << segment.start_addr_ << ", " << segment.end_addr_ << ')';
    return os;
  }
};

// Base class of ObjectFile.
class ObjectFileBase
{
 protected:
  // Address index for all currently discovered loadable ELF segments. The map key is the segment's one-past-the-end address;
  // the pointed-to PTLoadSegment stores the matching start address, flags, and a pointer to the ObjectFile.
  using object_files_t = threadsafe::Unlocked<std::map<std::uintptr_t, PTLoadSegment const>, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
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
