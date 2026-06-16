@page reference-manual Reference Manual

\section reference-manual-getting-started Configuration, Installation And Getting Started

Start here when installing libcwd, preparing the project-local debug headers, selecting CMake configuration options,
or checking the environment variables that can influence runtime behavior.

- @subpage downloading
- @subpage preparation
- @subpage chapter_custom_debug_h
- @ref group_configuration
- @subpage chapter_environment

\section reference-manual-writing-debug-output Writing Debug Output

This chapter collects the output destination, debug object, debug channel, control flag, formatting, fatal output,
nesting, and helper utility documentation used while writing debug output.

- @subpage book_writing_intro
- @subpage page_why_macro
- @subpage destination
- @subpage debug_object
  - @subpage chapter_custom_do
- @subpage debug_channels
  - @ref group_default_dc
- @subpage control_flags
- @subpage formatting
- @subpage fatal_output
- @subpage chapter_nesting
- @subpage special

\section reference-manual-symbols Symbols Access And Interpretation

This chapter covers source locations, symbol lookup, demangling, and type introspection facilities backed by the DWARF
and ELF information available to libcwd.

- @subpage chapter_symbols_intro
- @subpage locations
- @subpage type_info
  - @ref group_demangle
- @subpage function_objects

\section reference-manual-runtime Miscellaneous Runtime

This chapter gathers runtime support that does not belong to ordinary debug output, including rcfile processing,
attaching gdb, forcing core dumps, and helper functions for gdb sessions.

- @subpage rcfile
- @subpage chapter_attach_gdb
- @subpage core_dump
- @subpage chapter_gdb

#### Generated API groups

- @ref group_configuration
- @ref group_destination
- @ref group_debug_object
- @ref group_debug_channels
  - @ref group_default_dc
- @ref group_control_flags
- @ref group_formatting
- @ref group_fatal_output
- @ref group_special
- @ref group_locations
- @ref group_type_info
  - @ref group_demangle
- @ref group_function
- @ref chapter_rcfile
- @ref chapter_core_dump
