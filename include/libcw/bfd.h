// $Header$
//
// Copyright (C) 2000 - 2002, by
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
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_BFD_H
#define LIBCW_BFD_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CLASS_LOCATION_H
#include <libcw/class_location.h>
#endif
#ifndef LIBCW_PC_MANGLED_FUNCTION_NAME_H
#include <libcw/pc_mangled_function_name.h>
#endif

#ifdef CWDEBUG_DLOPEN_DEFINED
#include <dlfcn.h>
#define dlopen __libcwd_dlopen
#define dlclose __libcwd_dlclose
extern "C" void* dlopen(char const*, int);
extern "C" int dlclose(void*);
#endif // CWDEBUG_DLOPEN_DEFINED

// Include the inline functions.
#ifndef LIBCW_CLASS_LOCATION_INL
#include <libcw/class_location.inl>
#endif

#endif // LIBCW_BFD_H
