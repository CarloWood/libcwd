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

#ifndef LIBCW_PERF_H
#define LIBCW_PERF_H

RCSTAG_H(perf, "$Id$")

#include <papi.h>

extern void PERFstats_init(char const* name, bool plot);
extern void PERFstats_store(void);
extern void PERFstats_plot(int certainty_index);

extern long long PERF_result_list[4];	// Array to temporally store the counter results
extern int PERF_eventset_count;		// Currently measured event set.
extern int PERF_max_eventset_count;	// Number of initialized event sets.
extern int PERF_eventsets[64];		// Event sets.

// Initialization of the above four variables (call once):
#define PERF_INIT_PLOT(name) PERFstats_init(name, true)
#define PERF_INIT(name) PERFstats_init(name, false)

#define PERF_START \
  PAPI_start(PERF_eventsets[(PERF_eventset_count = ((PERF_eventset_count + 1 == PERF_max_eventset_count) ? 0 : PERF_eventset_count + 1))])

#define PERF_STOP \
  do { \
    if (PAPI_stop(PERF_eventsets[PERF_eventset_count], &PERF_result_list[0]) < PAPI_OK) \
      __DoutFatal( dc::fatal, "Problems with stopping performance counters" ); \
    PERFstats_store(); \
  } while(0)

#define PERF_PLOT(i) PERFstats_plot(i)

#endif // LIBCW_PERF_H
