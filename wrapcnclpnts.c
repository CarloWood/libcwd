#include <libcw/debug_config.h>		// Needed for LIBCWD_DEBUG_THREADS

#if LIBCWD_DEBUG_THREADS

#define _LARGEFILE64_SOURCE
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

#define CANCELABLE_SYSCALL(res_type, name, param_list, params) \
res_type __libc_##name param_list;					      \
res_type								      \
__attribute__ ((weak))							      \
name param_list								      \
{									      \
  res_type result;							      \
  int oldtype;								      \
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);	      \
  result = __libc_##name params;					      \
  pthread_setcanceltype (oldtype, NULL);				      \
  return result;							      \
}

#define CANCELABLE_SYSCALL_VA(res_type, name, param_list, params, last_arg) \
res_type __libc_##name param_list;					      \
res_type								      \
__attribute__ ((weak))							      \
name param_list								      \
{									      \
  res_type result;							      \
  int oldtype;								      \
  va_list ap;								      \
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);	      \
  va_start (ap, last_arg);						      \
  result = __libc_##name params;					      \
  va_end (ap);								      \
  pthread_setcanceltype (oldtype, NULL);				      \
  return result;							      \
}

#define strong_alias(name, aliasname) extern __typeof (name) aliasname __attribute__ ((alias (#name)));

CANCELABLE_SYSCALL (int, close, (int fd), (fd))
strong_alias (close, __close)

CANCELABLE_SYSCALL_VA (int, fcntl, (int fd, int cmd, ...), (fd, cmd, va_arg (ap, long int)), cmd)
strong_alias (fcntl, __fcntl)

CANCELABLE_SYSCALL (int, fsync, (int fd), (fd))

CANCELABLE_SYSCALL (off_t, lseek, (int fd, off_t offset, int whence), (fd, offset, whence))
strong_alias (lseek, __lseek)

CANCELABLE_SYSCALL (off64_t, lseek64, (int fd, off64_t offset, int whence), (fd, offset, whence))

CANCELABLE_SYSCALL (int, msync, (__ptr_t addr, size_t length, int flags), (addr, length, flags))

CANCELABLE_SYSCALL (int, nanosleep, (const struct timespec *requested_time, struct timespec *remaining), (requested_time, remaining))

CANCELABLE_SYSCALL_VA (int, open, (const char *pathname, int flags, ...), (pathname, flags, va_arg (ap, mode_t)), flags)
strong_alias (open, __open)

CANCELABLE_SYSCALL_VA (int, open64, (const char *pathname, int flags, ...), (pathname, flags, va_arg (ap, mode_t)), flags)
strong_alias (open64, __open64)

CANCELABLE_SYSCALL (int, pause, (void), ())

CANCELABLE_SYSCALL (ssize_t, pread, (int fd, void *buf, size_t count, off_t offset), (fd, buf, count, offset))

CANCELABLE_SYSCALL (ssize_t, pread64, (int fd, void *buf, size_t count, off64_t offset), (fd, buf, count, offset))
strong_alias (pread64, __pread64)

CANCELABLE_SYSCALL (ssize_t, pwrite, (int fd, const void *buf, size_t n, off_t offset), (fd, buf, n, offset))

CANCELABLE_SYSCALL (ssize_t, pwrite64, (int fd, const void *buf, size_t n, off64_t offset), (fd, buf, n, offset))
strong_alias (pwrite64, __pwrite64)

CANCELABLE_SYSCALL (ssize_t, read, (int fd, void *buf, size_t count), (fd, buf, count))
strong_alias (read, __read)

CANCELABLE_SYSCALL (int, system, (const char *line), (line))

CANCELABLE_SYSCALL (int, tcdrain, (int fd), (fd))

CANCELABLE_SYSCALL (__pid_t, wait, (__WAIT_STATUS_DEFN stat_loc), (stat_loc))
strong_alias (wait, __wait)

CANCELABLE_SYSCALL (__pid_t, waitpid, (__pid_t pid, int *stat_loc, int options), (pid, stat_loc, options))

CANCELABLE_SYSCALL (ssize_t, write, (int fd, const void *buf, size_t n), (fd, buf, n))
strong_alias (write, __write)

CANCELABLE_SYSCALL (int, accept, (int fd, __SOCKADDR_ARG addr, socklen_t *addr_len), (fd, addr, addr_len))

CANCELABLE_SYSCALL (int, connect, (int fd, __CONST_SOCKADDR_ARG addr, socklen_t len), (fd, addr, len))
strong_alias (connect, __connect)

CANCELABLE_SYSCALL (ssize_t, recv, (int fd, __ptr_t buf, size_t n, int flags), (fd, buf, n, flags))

CANCELABLE_SYSCALL (ssize_t, recvfrom, (int fd, __ptr_t buf, size_t n, int flags, __SOCKADDR_ARG addr, socklen_t *addr_len), (fd, buf, n, flags, addr, addr_len))

CANCELABLE_SYSCALL (ssize_t, recvmsg, (int fd, struct msghdr *message, int flags), (fd, message, flags))

CANCELABLE_SYSCALL (ssize_t, send, (int fd, const __ptr_t buf, size_t n, int flags), (fd, buf, n, flags))
strong_alias (send, __send)

CANCELABLE_SYSCALL (ssize_t, sendmsg, (int fd, const struct msghdr *message, int flags), (fd, message, flags))

CANCELABLE_SYSCALL (ssize_t, sendto, (int fd, const __ptr_t buf, size_t n, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len), (fd, buf, n, flags, addr, addr_len))

#endif // LIBCWD_DEBUG_THREADS
