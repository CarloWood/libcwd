// sys.h
//
// This header file is included at the top of every source file,
// before any other header file.
//
// It is intended to add defines that are needed globally and
// to work around Operating System dependent incompatibilities.

// The following includes the cwds related mandatory part,
// which must be included before any system header file is included!

#define LIBCWD_VERSION_2
#include "cwds/sys.h"           // Among others, this will include config.h header files.

// Add project specific parts here.
