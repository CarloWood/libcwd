/**
 * ai-utils -- C++ Core utilities
 *
 * @file
 * @brief Declaration of frequently used macros.
 *
 * @Copyright (C) 2017  Carlo Wood.
 *
 * pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
 * fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
 *
 * This file is part of ai-utils.
 *
 * ai-utils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ai-utils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ai-utils.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/expand.hpp>

#if defined(CWDEBUG) && !defined(LIBCWD_SYS_H)
// We need sys.h included because that includes config.h, which defines HAVE_BUILTIN_EXPECT.
#error #include "sys.h" at the top of every source file!
#endif

#if HAVE_BUILTIN_EXPECT
#define AI_LIKELY(EXPR) __builtin_expect (static_cast<bool>(EXPR), true)
#define AI_UNLIKELY(EXPR) __builtin_expect (static_cast<bool>(EXPR), false)
#else
#define AI_LIKELY(EXPR) (EXPR)
#define AI_UNLIKELY(EXPR) (EXPR)
#endif

#define AI_CASE_RETURN(x) do { case x: return #x; } while(0)

#if defined(__GNUC__) && !defined(__clang__) // clang doesn't have a -Wmaybe-uninitialized warning.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_maybe_uninitialized \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_maybe_uninitialized \
  _Pragma("GCC diagnostic push")
#endif

#if defined(__GNUC__) && !defined(__clang__) // clang doesn't have a -Wframe-address warning.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wframe-address\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address \
  _Pragma("GCC diagnostic push")
#endif

#if defined(__GNUC__) && !defined(__clang__) // clang doesn't have a -Wnon-template-friend warning.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_non_template_friend \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wnon-template-friend\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_non_template_friend \
  _Pragma("GCC diagnostic push")
#endif

#if defined(__GNUC__) && defined(__clang__) // gcc doesn't have a -Wnullability-completeness.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_nullability_completeness \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wnullability-completeness\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_nullability_completeness \
  _Pragma("GCC diagnostic push")
#endif

#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE(warn_option) \
  _Pragma("GCC diagnostic push") \
  _Pragma(BOOST_PP_STRINGIZE(GCC diagnostic ignored BOOST_PP_EXPAND(warn_option)))

#define PRAGMA_DIAGNOSTIC_POP \
  _Pragma("GCC diagnostic pop")

// Signed sizeof.
#define ssizeof (long)sizeof
