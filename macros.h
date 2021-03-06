#pragma once

#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__) // clang doesn't have a -Wclass-memaccess warning.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_class_memaccess \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wclass-memaccess\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_class_memaccess \
  _Pragma("GCC diagnostic push")
#endif

#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__) // clang doesn't have a -W warning.
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_stringop_overflow \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wstringop-overflow\"") \
  _Pragma("GCC diagnostic ignored \"-Wstringop-truncation\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_stringop_overflow \
  _Pragma("GCC diagnostic push")
#endif

#if (defined(__GNUC__) && __GNUC__ >= 8) || defined(__clang__)
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wframe-address\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address \
  _Pragma("GCC diagnostic push")
#endif

#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_infinite_recursion \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Winfinite-recursion\"")

#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_mismatched_new_delete \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wmismatched-new-delete\"")

#ifdef __GNUC__
#define PRAGMA_DIAGNOSTIC_POP \
  _Pragma("GCC diagnostic pop")
#endif
