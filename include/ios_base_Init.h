// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef IOS_BASE_INIT_H
#define IOS_BASE_INIT_H

#if CWDEBUG_ALLOC

namespace libcw {
  namespace debug {
    namespace _private_ {

extern bool WST_ios_base_initialized;
extern bool inside_ios_base_Init_Init(void);

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // CWDEBUG_ALLOC

#endif // IOS_BASE_INIT__H
