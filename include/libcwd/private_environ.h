// $Header$
//
// Copyright (C) 2002 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_PRIVATE_ENVIRON_H
#define LIBCWD_PRIVATE_ENVIRON_H

#ifndef LIBCWD_SYS_H
#error "You need to #include "sys.h" at the top of every source file (which in turn should #include <libcwd/sys.h>)."
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

extern void process_environment_variables(void);

// Environment variable: LIBCWD_PRINT_LOADING
// Print the list with "BFD     : Loading debug info from /lib/libc.so.6 (0x40271000) ... done (4189 symbols)" etc.
// at the start of the program *even* when this happens before main() is reached and libcw_do and dc::bfd are
// still turned off.
extern bool always_print_loading;

// Environment variable: LIBCWD_NO_STARTUP_MSGS
// This will suppress all messages that normally could be printed
// before reaching main, including warning messages.
// This overrides LIBCWD_PRINT_LOADING.
extern bool suppress_startup_msgs;

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCWD_PRIVATE_ENVIRON_H
