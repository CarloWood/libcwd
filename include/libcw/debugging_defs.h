// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_DEBUGGING_DEFS_H
#define LIBCW_DEBUGGING_DEFS_H

// Read the README file in this directory
//

//
//
// DEBUG
//
// In general, this enables debug output and adds several
// double checking in the code. The level of output can
// be choosen seperately when supported.
// In general, this should be defined as long as you are
// develloping the program, unless you want to do
// speed tests.

#ifdef __LIBCW__
#define DEBUG
#endif

//
//
// DEBUGMALLOC
//
// Turn on debugging for memory (de-) allocation. This
// will warn you when you free an invalid block.
// It also enables Memory Leak Detection (See ...).

#ifdef __LIBCW__
#define DEBUGMALLOC
#endif

//
//
// DEBUGMAGICMALLOC
//
// Add a magic number to the beginning and the end of each
// allocated memory block, and check this when the block is
// deleted or reallocated.

#ifdef DEBUGMALLOC
#define DEBUGMAGICMALLOC
#endif

//
//
// DEBUGUSEBFD
//
// Add BFD code.
//
// You will need libbfd.a and libiberty.a, which are part of GNU binutils.
// You'll need to add "-lbfd -liberty -ldl" to LDFLAGS in $LIBCWDIR/Makedefs.h.
//
// Turning this on causes libcw to print source file and line
// number information about where memory was allocated in the
// memory allocation overview.
// It also provides an interface, allowing applications that link
// with libcw to print debugging information using the symbol table.
//

#ifdef DEBUG
#undef DEBUGUSEBFD
#endif

//
//
// DEBUGDEBUG
//
// Turns on a lot of extra debugging output concerning
// global declarations prior to the initialisation of the
// debug channels and concerning the 'alloc_list_ct'
// structure used in debugmalloc (if DEBUGMALLOC is
// defined). This will not be usefull to you unless
// you manage to let the program coredump even before
// it reaches main().
// An added feature: With this defined, your program passes
// debugdebugcheckpoint() every time you use the Dout()
// macro, this is intended to be used to find a point in the
// program close to an unknown bug (before main()), and thus
// before debugger watch points can be used).

#ifdef DEBUG
#undef DEBUGDEBUG
#endif

//
//
// DEBUGDEBUGMALLOC
//
// Turns on even more debug output: Internal memory allocations
// are written to cerr.  This is needed for debugging internal
// memory allocations which are done before libcw is initialized.
// (Example: On Solaris2.4 it turned out that at the termination
//  of the program, in exit(2), an invalid pointer was free-ed.
//  Defining DEBUGDEBUGMALLOC was needed to find out when this
//  memory block had been allocated, and what it was).

#ifdef DEBUGDEBUG
#define DEBUGDEBUGMALLOC
#endif

//
//
// DEBUGNONAMESPACE
//
// Puts everything of ::libcw::debug in ::std, needed when debugging
// the debug code because gdb can't deal with namespaces correctly yet.
//

#ifdef DEBUG
#undef DEBUGNONAMESPACE
#endif

//
//
#endif // LIBCW_DEBUGGING_DEFS_H
