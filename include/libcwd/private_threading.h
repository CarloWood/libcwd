// SPDX-FileCopyrightText: 2001-2005, 2008, 2010, 2017-2021, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_threading.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_THREADING_H
#define LIBCWD_PRIVATE_THREADING_H

#include "private_struct_TSD.h"
#include "core_dump.h"

#ifdef LIBCWD_HAVE_PTHREAD
#include <pthread.h>
#else
#error Fatal error: thread support was not detected during configuration of libcwd.
#endif // LIBCWD_HAVE_PTHREAD

#if CWDEBUG_DEBUGT
#define LibcwDebugThreads(x) do { x; } while(0)
#else
#define LibcwDebugThreads(x) do { } while(0)
#endif

#endif // LIBCWD_PRIVATE_THREADING_H
