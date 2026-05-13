/**
 * threadsafe -- Threading utilities: object oriented (read/write) locking and more.
 *
 * @file
 * @brief Declarations related to std::thread::id.
 *
 * @Copyright (C) 2015 - 2017  Carlo Wood.
 *
 * RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
 * Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
 *
 * This file is part of threadsafe.
 *
 * Threadsafe is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Threadsafe is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with threadsafe.  If not, see <http://www.gnu.org/licenses/>.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   2015/02/27
 *   - Initial version, written by Aleric Inglewood @ SL
 *
 *   2016/12/17
 *   - Transfered copyright to Carlo Wood.
 */

#pragma once

#include "utils/macros.h"
#include <thread>

namespace aithreadid
{

extern std::thread::id const none;
extern std::thread::id const s_main_thread_id;

// Debugging function.
// Usage:
//
//   static std::thread::id s_id;
//   assert(aithreadid::is_single_threaded(s_id));	// Fails if more than one thread executes this line.
inline bool is_single_threaded(std::thread::id& thread_id)
{
  if (AI_LIKELY(thread_id == std::this_thread::get_id()))
    return true;
  bool const first_call = thread_id == none;
  if (AI_LIKELY(first_call))
    thread_id = std::this_thread::get_id();
  return first_call;
}

// Debugging function.
// Usage:
//
//   assert(in_main_thread());                          // Fails if this is not the main thread. Only use after main() was reached.
inline bool in_main_thread()
{
  return s_main_thread_id == std::this_thread::get_id();
}

} // namespace aithreadid
