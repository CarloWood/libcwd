// $Header$
//
// Copyright (C) 2000 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file bfd.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_BFD_H
#define LIBCWD_BFD_H

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCWD_CLASS_LOCATION_H
#include "class_location.h"
#endif
#ifndef LIBCWD_PC_MANGLED_FUNCTION_NAME_H
#include "pc_mangled_function_name.h"
#endif

#if CWDEBUG_ALLOC
namespace libcwd {
  namespace _private_ {
    struct exit_function_list
    {
      struct exit_function_list* next;
      // More here...
    };
    extern struct exit_function_list** __exit_funcs_ptr;
  }
}
#endif

#endif // LIBCWD_BFD_H
