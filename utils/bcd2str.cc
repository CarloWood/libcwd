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
#include <libcw/bcd2str.h>

RCSTAG_CC("$Id$")

namespace libcw {
  namespace debug {
    namespace pu {

      char const* bcd2str(char const* buf, int len)
      {
	static char* internal_buf;

	Dout(dc::debug, "bcd2str(\"" << buf << "\", " << len << ")");
	if (internal_buf)
	  delete [] internal_buf;
	if ((internal_buf = NEW( char [2 * len + 1] )))
	{
	  for (int i = 0; i < len; i++)
	  {
	    internal_buf[2 * i] = (buf[i] >> 8) + '0';
	    internal_buf[2 * i + 1] = (buf[i] & 0xf) + '0';
	  }
	  internal_buf[2 * len] = '\0';
	}
	return internal_buf;
      }

    } // namespace pu
  } // namespace debug
} // namespace libcw
