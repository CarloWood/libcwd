// $Header$
//
// Copyright (C) 2000 - 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_EXEC_PROG_H
#define LIBCW_EXEC_PROG_H

namespace libcw {
  namespace debug {

extern int ST_exec_prog(char const* prog_name, char const* const argv[], char const* const envp[], int (*decode_stdout)(char const*, size_t));

  } // namespace debug
} // namespace libcw

#endif // LIBCW_EXEC_PROG_H
