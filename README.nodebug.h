In order to make it possible that others compile your application without
having libcwd installed, the file nodebug.h must be part of the distribution
of your application and should be included instead of <libcw/debug.h>
when CWDEBUG is not defined.

For an example of how to do this, see the example-project directory.

