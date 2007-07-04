// $Header$
//
// Copyright (C) 2005, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef ZONE_H
#define ZONE_H

#if CWDEBUG_MAGIC

// The size of the 'prezone' struct must be a multiple of 8 bytes, because it's
// size will be added to the value returned by malloc(3) -- and programs might
// depend on the fact that glibc's malloc returns memory aligned to 8 bytes.
//
// In the case of a minor underflow (assuming that contigious memory corruption
// is the most likely), we don't want the 'size' to be corrupted and the 'magic'
// not; therefore the magic needs to come after the 'size'. Because a size_t
// (for the 'size') might be 8 bytes, we can't use a fixed 32 bit magic: it
// wouldn't be aligned: the 'magic' also has to be of type 'size_t'.

// LIBCWD_REDZONE should be a positive integer, being the minimum size
// of redzones in bytes; It's defined during configure with "--with-redzone=value".
#define LIBCWD_REDZONE_BLOCKS (((LIBCWD_REDZONE) + 7) / 8)

namespace libcwd {

#if LIBCWD_REDZONE_BLOCKS > 0
// This will be even when sizeof(size_t) is 4 (LIBCWD_REDZONE_BLOCKS * 2), still resulting in
// a redzone of a multiple of 8 bytes. In the case sizeof(size_t) is larger than 8,
// the actual size in bytes will be rounded up.
static int const redzone_size = ((int)LIBCWD_REDZONE_BLOCKS * 8 - 1) / sizeof(size_t) + 1;
#endif

struct prezone {
  size_t magic;
  size_t size;		// Requested size + 2 * OFFSET (see debugmalloc.cc)
#if LIBCWD_REDZONE_BLOCKS > 0
  size_t redzone[redzone_size];
#endif
};

struct postzone {
#if LIBCWD_REDZONE_BLOCKS > 0
  size_t redzone[redzone_size];
#endif
  size_t magic;
};

} // namespace libcwd

#endif // CWDEBUG_MAGIC
#endif // ZONE_H
