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

#ifndef LIBCW_BFD_H
#define LIBCW_BFD_H

#ifndef DEBUGUSEBFD

RCSTAG_H(nobfd, "$Id$")

#else // DEBUGUSEBFD

RCSTAG_H(bfd, "$Id$")

#ifdef DEBUG
#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    namespace dc {
      extern channel_ct const bfd;
    };
#ifndef DEBUGNONAMESPACE
  };
};
#endif
#endif

#include <bfd.h>

struct location_st {
  char const* file;		// NULL or allocated with malloc().
  unsigned int line;
  char const* func;		// Pointer to static string.
};

extern ostream& operator<<(ostream& o, location_st const& loc);
extern location_st libcw_bfd_pc_location(void const* addr);
extern char const* libcw_bfd_pc_function_name(void const* addr);

#endif // DEBUGUSEBFD

#endif // LIBCW_BFD_H
