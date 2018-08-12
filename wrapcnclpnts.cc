// $Header$
//
// Copyright (C) 2002 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "sys.h"
#include <libcwd/config.h>		// Needed for CWDEBUG_DEBUGT

#if CWDEBUG_DEBUGT && defined(__linux)

#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <dlfcn.h>

#define CANCELLATION_POINT_SYSCALL(return_t, name, arg_list, args)			\
return_t __##name arg_list;								\
return_t __attribute__ ((weak)) name arg_list						\
{											\
  LIBCWD_TSD_DECLARATION;								\
  LIBCWD_ASSERT( __libcwd_tsd.inside_critical_area == __libcwd_tsd.cleanup_handler_installed || __libcwd_tsd.cancel_explicitely_disabled || __libcwd_tsd.internal_debugging_code );	\
  int oldtype;										\
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);			\
  return_t res = __##name args;								\
  pthread_setcanceltype(oldtype, NULL);							\
  return res;										\
}

// glibc 2.3.3 did hide the __libc_* functions, and some functions lack an alias with a double underscore prefix.
#define CANCELLATION_POINT_SYSCALL_DLSYM(return_t, name, arg_list, args)		\
typedef return_t (*name##_type) arg_list;						\
name##_type real_##name;								\
return_t __attribute__ ((weak)) name arg_list						\
{											\
  if (!real_##name)									\
    real_##name = (name##_type)dlsym(RTLD_NEXT, #name);					\
  LIBCWD_TSD_DECLARATION;								\
  LIBCWD_ASSERT( __libcwd_tsd.inside_critical_area == __libcwd_tsd.cleanup_handler_installed || \
      __libcwd_tsd.cancel_explicitely_disabled || __libcwd_tsd.internal_debugging_code );	\
  int oldtype;										\
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);			\
  return_t res = real_##name args;							\
  pthread_setcanceltype(oldtype, NULL);							\
  return res;										\
}

#define CANCELLATION_POINT_SYSCALL_VA(return_t, name, arg_list, args, last_arg)		\
return_t __##name arg_list;								\
return_t __attribute__ ((weak)) name arg_list						\
{											\
  LIBCWD_TSD_DECLARATION;								\
  LIBCWD_ASSERT( __libcwd_tsd.inside_critical_area == __libcwd_tsd.cleanup_handler_installed || __libcwd_tsd.cancel_explicitely_disabled || __libcwd_tsd.internal_debugging_code );	\
  int oldtype;										\
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);			\
  va_list ap;										\
  va_start(ap, last_arg);								\
  return_t res = __##name args;								\
  va_end(ap);										\
  pthread_setcanceltype(oldtype, NULL);							\
  return res;										\
}

#define strong_alias(name, aliasname) extern __typeof (name) aliasname __attribute__ ((alias (#name)));

extern "C" {

CANCELLATION_POINT_SYSCALL (int, close, (int fd), (fd))
//strong_alias(close, __close)

CANCELLATION_POINT_SYSCALL_VA (int, fcntl, (int fd, int cmd, ...), (fd, cmd, va_arg (ap, long int)), cmd)
//strong_alias(fcntl, __fcntl)

CANCELLATION_POINT_SYSCALL_DLSYM (int, fsync, (int fd), (fd))

CANCELLATION_POINT_SYSCALL (off_t, lseek, (int fd, off_t offset, int whence), (fd, offset, whence))
//strong_alias(lseek, __lseek)

CANCELLATION_POINT_SYSCALL_DLSYM (off64_t, lseek64, (int fd, off64_t offset, int whence), (fd, offset, whence))

CANCELLATION_POINT_SYSCALL_DLSYM (int, msync, (__ptr_t addr, size_t length, int flags), (addr, length, flags))

CANCELLATION_POINT_SYSCALL (int, nanosleep, (const struct timespec *requested_time, struct timespec *remaining), (requested_time, remaining))

CANCELLATION_POINT_SYSCALL_VA (int, open, (const char *pathname, int flags, ...), (pathname, flags, va_arg (ap, mode_t)), flags)
//strong_alias(open, __open)

CANCELLATION_POINT_SYSCALL_VA (int, open64, (const char *pathname, int flags, ...), (pathname, flags, va_arg (ap, mode_t)), flags)
//strong_alias(open64, __open64)

CANCELLATION_POINT_SYSCALL_DLSYM (int, pause, (), ())

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, pread, (int fd, void *buf, size_t count, off_t offset), (fd, buf, count, offset))

CANCELLATION_POINT_SYSCALL (ssize_t, pread64, (int fd, void *buf, size_t count, off64_t offset), (fd, buf, count, offset))
//strong_alias(pread64, __pread64)

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, pwrite, (int fd, const void *buf, size_t n, off_t offset), (fd, buf, n, offset))

CANCELLATION_POINT_SYSCALL (ssize_t, pwrite64, (int fd, const void *buf, size_t n, off64_t offset), (fd, buf, n, offset))
//strong_alias(pwrite64, __pwrite64)

CANCELLATION_POINT_SYSCALL (ssize_t, read, (int fd, void *buf, size_t count), (fd, buf, count))
//strong_alias(read, __read)

CANCELLATION_POINT_SYSCALL_DLSYM (int, system, (const char *line), (line))

CANCELLATION_POINT_SYSCALL_DLSYM (int, tcdrain, (int fd), (fd))

CANCELLATION_POINT_SYSCALL (__pid_t, wait, (__WAIT_STATUS_DEFN stat_loc), (stat_loc))
//strong_alias(wait, __wait)

// We can't wrap this call because that leads to a deadlock when threads are
// being cancelled.  I don't think it is needed anyway because libcwd doesn't
// call waitpid anywhere.
// CANCELLATION_POINT_SYSCALL (__pid_t, waitpid, (__pid_t pid, int *stat_loc, int options), (pid, stat_loc, options))

CANCELLATION_POINT_SYSCALL (ssize_t, write, (int fd, const void *buf, size_t n), (fd, buf, n))
//strong_alias(write, __write)

CANCELLATION_POINT_SYSCALL_DLSYM (int, accept, (int fd, __SOCKADDR_ARG addr, socklen_t *addr_len), (fd, addr, addr_len))

CANCELLATION_POINT_SYSCALL (int, connect, (int fd, __CONST_SOCKADDR_ARG addr, socklen_t len), (fd, addr, len))
//strong_alias(connect, __connect)

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, recv, (int fd, __ptr_t buf, size_t n, int flags), (fd, buf, n, flags))

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, recvfrom, (int fd, __ptr_t buf, size_t n, int flags, __SOCKADDR_ARG addr, socklen_t *addr_len), (fd, buf, n, flags, addr, addr_len))

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, recvmsg, (int fd, struct msghdr *message, int flags), (fd, message, flags))

CANCELLATION_POINT_SYSCALL (ssize_t, send, (int fd, const __ptr_t buf, size_t n, int flags), (fd, buf, n, flags))
strong_alias(send, __send)

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, sendmsg, (int fd, const struct msghdr *message, int flags), (fd, message, flags))

CANCELLATION_POINT_SYSCALL_DLSYM (ssize_t, sendto, (int fd, const __ptr_t buf, size_t n, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len), (fd, buf, n, flags, addr, addr_len))

} // extern "C"

#endif // CWDEBUG_DEBUGT && defined(__linux)
