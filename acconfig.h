
/* The `config.h' header file is only needed to compile libcw[d] itself.
   Configurations that are needed in header files of libcw[d], and
   therefore also needed when *using* libcw[d] are automatically copied
   into a different headerfile: src/libcwd/include/libcw/sys[d].h.  */

#ifndef LIBCW_CONFIG_H
#define LIBCW_CONFIG_H
@TOP@
// Full path to the `ps' executable.
#undef CW_PATH_PROG_PS

// Defined if dlopen is available.
#undef HAVE_DLOPEN

// This should be the argument to ps, causing it to print a wide output of a specified PID.
#undef PS_ARGUMENT

// Defined when memory access need to be aligned to sizeof(size_t) bytes alignment.
#undef LIBCWD_NEED_WORD_ALIGNMENT

// Defined when both -lpthread and pthread.h could be found.
#undef LIBCWD_HAVE_PTHREAD

// This is needed when using threading, for example to get `pthread_kill_other_threads_np'.
#undef _GNU_SOURCE

// Defined when --enable-libcwd-threading is used.
#undef LIBCWD_THREAD_SAFE
@BOTTOM@

#endif // LIBCW_CONFIG_H
