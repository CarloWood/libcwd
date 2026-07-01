#pragma once

/** @file
 * Do not include this header file directly, instead include @ref the-custom-debug-h-file "debug.h".
 */

//=============================================================================
// Macros that are normally a part of cwds.

// Test sources include cwd_sys.h first, optionally define the namespace
// macro NAMESPACE_DEBUG, and then include this header.
//
// This configures LIBCWD_DEBUG_CHANNELS before processing <libcwd/debug.h>,
// declares the application debug-channel namespace, and exposes
// NAMESPACE_DEBUG_CHANNELS_START/END for defining custom channels.

#ifndef NAMESPACE_DEBUG
#define NAMESPACE_DEBUG debug
#endif

// It should never be necessary for the user to define NAMESPACE_DEBUG_START/END.
#ifndef NAMESPACE_DEBUG_START
#define NAMESPACE_DEBUG_START namespace NAMESPACE_DEBUG {
#define NAMESPACE_DEBUG_END }
#endif

#ifndef NAMESPACE_CHANNELS
/**
 * The namespace name in which the `dc` namespace is declared.
 *
 * Define this macro before <libcwd/debug.h> is included; it must be the name of the namespace containing
 * the `dc` (Debug Channels) namespace. If not defined it defaults to `channels`.
 *
 * Note: you probably will never need this.
 *
 * @sa debug::channels::dc
 */
#define NAMESPACE_CHANNELS channels
#endif

// The real code

// It should never be necessary for the user to define LIBCWD_DEBUG_CHANNELS.
#ifndef LIBCWD_DEBUG_CHANNELS
/**
 * @brief The namespace containing the current %debug %channels (dc) namespace.
 *
 * The nested namespace in which the `dc` namespace is declared.
 *
 * Define this macro before <libcwd/debug.h> is included; it must be the (nested) name of the namespace containing
 * the `dc` (Debug Channels) namespace. If not defined it defaults to `NAMESPACE_DEBUG::NAMESPACE_CHANNELS`.
 *
 * Note: you probably will never need this (define NAMESPACE_DEBUG instead).
 *
 * @sa debug::channels::dc
 */
#define LIBCWD_DEBUG_CHANNELS NAMESPACE_DEBUG::NAMESPACE_CHANNELS
#endif

#define NAMESPACE_DEBUG_CHANNELS_START namespace LIBCWD_DEBUG_CHANNELS::dc {
#define NAMESPACE_DEBUG_CHANNELS_END }

// End of cwds macros.
//=============================================================================
