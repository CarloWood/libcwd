// Piggyback on cwds

#pragma once

#define NAMESPACE_DEBUG myproject::debug
#define NAMESPACE_DEBUG_START namespace NAMESPACE_DEBUG {
#define NAMESPACE_DEBUG_END }

#include "cwds/debug.h"

#ifdef CWDEBUG

// Declare project-wide debug channels here, or in some other (more) appropriate header file.
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct custom;
NAMESPACE_DEBUG_CHANNELS_END

// And define them in a .cpp file like:
//NAMESPACE_DEBUG_CHANNELS_START
//channel_ct custom("CUSTOM");
//NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
