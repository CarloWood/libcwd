@defgroup reference-manual Reference Manual

The main feature of Version 2 of this library is debug output. There is also support for locations (filename:line-number). As a first-time user, you should read the **Tutorial**, whereas this reference manual is intended to refresh your mind. Nevertheless, it also has a section on **Getting Started** with an overview of prerequisites and configuration options.

@internal This file determines the navigation tree structure and the order in which siblings appear in it. @endinternal
<!-- Reference Manual -->

  <!-- Configuration, Installation And Getting Started -->
@defgroup reference-manual-getting-started Configuration, Installation And Getting Started
@ingroup reference-manual
@brief Prerequisites for using libcwd.

@defgroup downloading Downloading
@ingroup reference-manual-getting-started

@defgroup preparation Preparation
@ingroup reference-manual-getting-started

@defgroup chapter_custom_debug_h The Custom debug.h File
@ingroup reference-manual-getting-started

@defgroup group_configuration Configuration Options And Macros
@ingroup reference-manual-getting-started
@brief CMake options and the preprocessor macros that expose those choices.

@defgroup chapter_environment Environment Variables
@ingroup reference-manual-getting-started

  <!-- Writing Debug Output -->
@defgroup reference-manual-writing-debug-output Writing Debug Output
@ingroup reference-manual
@brief API documentation related to producing debug output.

@defgroup book_writing_intro Introduction
@ingroup reference-manual-writing-debug-output

@defgroup page_why_macro Design Consideration Concerning Macros
@ingroup book_writing_intro

@defgroup group_destination Setting The Output Destination
@ingroup reference-manual-writing-debug-output
@brief Selecting or inspecting the ostream used by a debug object.

@defgroup group_debug_object The Output Device (Debug Object)
@ingroup reference-manual-writing-debug-output
@brief Documentation of `class libcwd::DebugObject` and the default debug object `libcwd::libcw_do`.

@defgroup chapter_custom_do Custom Debug Objects
@ingroup group_debug_object

@defgroup group_debug_channels Controlling The Output Level (Debug Channels)
@ingroup reference-manual-writing-debug-output
@brief Generated API reference for debug channels that can be turned on and off independently.

@defgroup group_default_dc Predefined Debug Channels
@ingroup group_debug_channels
@brief Generated API reference for the debug channels supplied by libcwd.

@defgroup group_control_flags Control Flags
@ingroup reference-manual-writing-debug-output
@brief Generated API reference for flags that alter how a debug output statement is written.

@defgroup group_formatting Format Of The Debug Output
@ingroup reference-manual-writing-debug-output
@brief Generated API reference for margins, markers, indentation, and other debug-output formatting helpers.

@defgroup group_fatal_output Fatal Debug Output
@ingroup reference-manual-writing-debug-output
@brief Generated API reference for output facilities that terminate the application after printing a fatal message.

@defgroup chapter_nesting Nesting Debug Output
@ingroup reference-manual-writing-debug-output

@defgroup group_special Special Functions And Utilities
@ingroup reference-manual-writing-debug-output
@brief Generated API reference for miscellaneous runtime helpers used with libcwd debug output.

  <!-- Symbols Access And Interpretation -->
@defgroup reference-manual-symbols Function Symbols, Types And Locations.
@ingroup reference-manual
@brief API documentation related to accessing DWARF debug info and ELF symbols.

@defgroup chapter_symbols_intro Introduction
@ingroup reference-manual-symbols

@defgroup group_locations Source-file:Line-number Information
@ingroup reference-manual-symbols
@brief Generated API reference for translating program locations to source-file and line-number information.

@defgroup group_type_info Getting Type Information Of Types And Symbols
@ingroup reference-manual-symbols
@brief Generated API reference for symbol, type, and demangling support.

@defgroup group_demangle demangle_type() and demangle_symbol()
@ingroup group_type_info
@brief Generated API reference for demangling C++ type and symbol names.

@defgroup group_function Function Objects
@ingroup reference-manual-symbols

  <!-- Miscellaneous Runtime -->
@defgroup reference-manual-runtime Miscellaneous Runtime
@ingroup reference-manual
@brief Supplementary Documentation

@defgroup chapter_rcfile Runtime Configuration File (rcfile)
@ingroup reference-manual-runtime
@brief Using `read_rcfile()`.

@defgroup chapter_attach_gdb Starting A gdb Session From A Running Program
@ingroup reference-manual-runtime
@brief Using `attach_gdb()`.

@defgroup chapter_core_dump Making The Program Dump Core
@ingroup reference-manual-runtime
@brief Using `core_dump()`.
