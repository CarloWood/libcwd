
/* The `config.h' header file is only needed to compile libcw[d] itself.
   Configurations that are needed in header files of libcw[d], and
   therefore also needed when *using* libcw[d] are automatically copied
   into a different headerfile: src/libcwd/include/libcw/sys.h.  */

#ifndef LIBCW_CONFIG_H
#define LIBCW_CONFIG_H
@TOP@
// Defined when RETSIGTYPE is `int'
#undef CW_RETSIGTYPE_IS_INT

// Full path to the `ps' executable
#undef CW_PATH_PROG_PS

// Define this if you have dlopen (and link with the appropriate library)
#undef HAVE_DLOPEN
@BOTTOM@

#endif // LIBCW_CONFIG_H
