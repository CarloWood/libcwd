#pragma once

// Define this if you want to use a different namespace than ::debug for this.
#define NAMESPACE_DEBUG myproject::debug

// Piggyback on cwds
#include "cwds/debug.h"

#ifdef CWDEBUG

// For example, declare project-wide debug channels here.
// Of course you can also declare them a more appropriate
// header file, more closely related to the debug channel
// itself.
NAMESPACE_DEBUG_CHANNELS_START
extern Channel custom;
NAMESPACE_DEBUG_CHANNELS_END

// Don't forget to define them in a TU:
//
//   NAMESPACE_DEBUG_CHANNELS_START
//   Channel custom("CUSTOM");
//   NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
