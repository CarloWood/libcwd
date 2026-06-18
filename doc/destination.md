@addtogroup group_destination

Basically, a %debug object (\link libcwd::DebugObject DebugObject \endlink) is a pointer to an ostream
with a few extra attributes added to give it an internal state for \link libcwd::DebugObject::on() on \endlink
(pass output on) and \link libcwd::DebugObject::off() off \endlink (don't pass output on)
as well as some \link group_formatting formatting \endlink information of how to write the
data that is passed on to its ostream.

The methods of \link libcwd::DebugObject DebugObject \endlink given here allow you to set
or get this ostream (pointer).
