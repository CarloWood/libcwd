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

#include <libcw/sys.h>
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/return_address.h>

RCSTAG_CC("$Id$")

//
// libcw_return_address
//
// Returns the same as __builtin_return_address.
// Some Operating Systems (so far only IRIX), do not support
// __builtin_return_address with an argument larger than 0,
// they have to use this function.

void* libcw_return_address(unsigned int frame)
{
  for (void* frame_ptr = *reinterpret_cast<void**>(__builtin_frame_address(0)); frame_ptr; frame_ptr = *reinterpret_cast<void**>(frame_ptr))
    if (frame-- == 0)
      return reinterpret_cast<void**>(frame_ptr)[1];
  return NULL;
}
