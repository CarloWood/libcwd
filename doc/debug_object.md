@addtogroup group_debug_object

Libcwd declares a %debug class (@ref libcwd::DebugObject "DebugObject") which can be assigned an <code>ostream</code>
 to which %debug output will be written.&nbsp;
Such a %debug object can dynamically be turned \link libcwd::DebugObject::on on \endlink
and \link libcwd::DebugObject::off off \endlink.&nbsp;
When the %debug object is turned off, no %debug output is written to its \link group_destination ostream \endlink;
in fact, the data that otherwise would be written is
not even evaluated (see @ref libcwd::DebugObject::on "example").&nbsp;
The %debug code can also completely be omitted, by not defining the macro CWDEBUG.

Libcwd defines and uses only one %debug object: @ref libcwd::libcw_do "libcw_do" (the _do stands for %Debug Object).
However, it is possible to \link chapter_custom_do create more \endlink %debug objects which would allow one to write %debug output to
two or more different output devices at the same time (for instance, the screen and a file).

For each %debug object it is possible to set a margin and a marker string and to set the size of the current indentation.&nbsp;
The methods used for setting or changing these prefix formatting attributes are listed in @ref group_formatting.
