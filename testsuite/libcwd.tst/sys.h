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

#ifndef TESTSUITE_SYS_H
#define TESTSUITE_SYS_H

#include "config.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libcwd/sys.h>

#ifdef HAVE__G_CONFIG_H
#include <_G_config.h>
#ifndef _G_CLOG_CONFLICT
#define _G_CLOG_CONFLICT 0
#endif
#ifndef _G_HAS_LABS
#define _G_HAS_LABS 1
#endif
#endif // HAVE__G_CONFIG_H

#ifndef THREADTEST
#define MAIN_FUNCTION int main(void)
#define EXIT(res) exit(res)
#define PREFIX_CODE
#define THREADED(x)
#define COMMA_THREADED(x)
#endif

#endif // TESTSUITE_SYS_H
