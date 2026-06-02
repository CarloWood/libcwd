// SPDX-FileCopyrightText: 2000-2006, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/macro_ForAllDebugObjects.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_FORALLDEBUGOBJECTS_H
#define LIBCWD_MACRO_FORALLDEBUGOBJECTS_H

#include "libcwd/config.h"
#include "private_assert.h"
#include <type_traits>

//===================================================================================================
// Macro ForAllDebugObjects
//

namespace libcwd {

class debug_ct;

namespace _private_ {

class debug_objects_ct
{
 public:
  using callback_type = void (*)(debug_ct&, void*);

 private:
  class impl_ct;
  impl_ct* M_impl;

 public:
  debug_objects_ct();
  ~debug_objects_ct();

  // Register debug_object in the debug-object registry unless it is already present.
  //
  // The registry is write-locked for the whole lookup-and-insert operation, so callers do not need to
  // acquire a separate lock or retry when another thread registers a debug object concurrently.
  void add_if_missing(debug_ct* debug_object);

  // Invoke func for each debug object currently registered in the registry.
  //
  // A read lock is held until all callbacks have returned. The callback receives debugObject as a
  // debug_ct reference; it must not try to register another debug object because that would require
  // upgrading the registry lock while this read access is still alive.
  template<typename Func>
  void for_each(Func&& func) const
  {
    using func_type = std::remove_reference_t<Func>;
    for_each_impl([](debug_ct& debugObject, void* data) { (*static_cast<func_type*>(data))(debugObject); }, &func);
  }

  void for_each_impl(callback_type callback, void* data) const;

  // The process-wide debug_objects() singleton is intentionally leaked because global destructors can
  // still touch libcwd after ordinary namespace-scope objects would have been destructed. The vector
  // contains non-owning pointers to debug_ct objects; debug_ct lifetime is managed by its callers.

  debug_objects_ct(debug_objects_ct const&) = delete;
  debug_objects_ct& operator=(debug_objects_ct const&) = delete;
};

// Return the process-wide debug-object registry.
//
// The registry is constructed on first use and intentionally never destroyed, which allows debug_ct
// constructors to use it even before namespace-scope objects in this translation unit have been
// initialized and avoids shutdown-order problems during global destruction.
debug_objects_ct& debug_objects();

} // namespace _private_
} // namespace libcwd

#define LibcwdForAllDebugObjects(dc_namespace, STATEMENT...) \
       do { \
	 ::libcwd::_private_::debug_objects().for_each([&](::libcwd::debug_ct& debugObject) { \
	   using namespace ::libcwd; \
	   using namespace dc_namespace; \
	   { STATEMENT; } \
	 }); \
       } \
       while(0)

#endif // LIBCWD_MACRO_FORALLDEBUGOBJECTS_H
