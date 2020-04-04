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
  _Pragma("GCC diagnostic ignored \"-Wstringop-overflow\"")
#else
#define PRAGMA_DIAGNOSTIC_PUSH_IGNORE_stringop_overflow \
  _Pragma("GCC diagnostic push")
#endif

#ifdef __GNUC__
#define PRAGMA_DIAGNOSTIC_POP \
  _Pragma("GCC diagnostic pop")
#endif
