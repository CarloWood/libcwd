// Stress core-dump debug output while all hardware-concurrent worker threads contend for the debug-output path.
//
// The parent process forks a child so the test can verify that DoutFatal(dc::core, ...) prints its fatal message
// and terminates the child through std::abort without making CTest itself abort. The child redirects stderr to a
// pipe, starts one worker per reported hardware thread, and releases them at the same gate so they first emit normal
// debug output and then all attempt DoutFatal(dc::core, ...).
//
// The SIGABRT handler in the child writes one byte to a dedicated pipe. std::abort then terminates the child with
// SIGABRT. The parent requires exactly one byte, proving that only one worker reached the abort signal path, and
// also checks that the fatal debug message reached stderr before termination.

#include "cwd_sys.h"
#include "StartingGate.h"
#include "test_support.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr std::string_view fatal_token = "fatal-concurrent-threads-core-dump";
int abort_signal_fd = -1;
std::mutex debug_output_mutex;

// Record that this process reached SIGABRT without using libc's write wrapper.
//
// The libcwd DEBUGT build interposes write(2), and that wrapper is not async-signal-safe.
// Calling the syscall directly keeps the signal handler restricted to a single best-effort byte write.
void record_abort_signal(int)
{
  if (abort_signal_fd != -1)
  {
    char const marker = 'A';
    syscall(SYS_write, abort_signal_fd, &marker, sizeof(marker));
  }
}

// Return the number of threads used by this stress test.
//
// std::thread::hardware_concurrency is the portable estimate of the maximum number of threads that can make
// progress concurrently. Some implementations may return zero; in that case still use two workers so the fatal
// path is genuinely multi-threaded.
int concurrent_thread_count()
{
  return static_cast<int>(std::max(2u, std::thread::hardware_concurrency()));
}

// Read all data that remains available on fd until EOF.
//
// Returns false only for read errors other than EINTR. The caller owns fd and remains responsible for closing it.
bool read_all_from_fd(int fd, std::string& out)
{
  char buffer[4096];
  for (;;)
  {
    ssize_t const bytes_read = ::read(fd, buffer, sizeof(buffer));
    if (bytes_read > 0)
    {
      out.append(buffer, static_cast<std::size_t>(bytes_read));
      continue;
    }
    if (bytes_read == 0)
      return true;
    if (errno == EINTR)
      continue;
    return false;
  }
}

// Wait for pid and store the resulting status.
//
// Returns false when waitpid itself fails. EINTR is retried because the parent might receive incidental signals
// while the child is tearing itself down through SIGABRT.
bool wait_for_child(pid_t pid, int& status)
{
  for (;;)
  {
    pid_t const result = ::waitpid(pid, &status, 0);
    if (result == pid)
      return true;
    if (result == -1 && errno == EINTR)
      continue;
    return false;
  }
}

// Emit normal and fatal debug output from one child worker.
//
// output_gate makes all workers contend for the normal debug stream at the same time. fatal_gate is reached only
// after that first debug line has completed, so every worker is poised at the fatal call before any worker can
// enter DoutFatal(dc::core, ...). Exactly one worker should then reach the abort signal path; other contenders
// should leave through libcwd's fatal-termination ownership check or be cut off by process termination.
void child_worker(int worker_index, utils::threading::StartingGate& output_gate,
                  utils::threading::StartingGate& fatal_gate)
{
  Debug(libcw_do.margin().assign("FATAL ", 6));
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  output_gate.wait();

  Dout(dc::notice, "worker " << worker_index << " reached concurrent fatal gate");
  fatal_gate.wait();
  DoutFatal(dc::core, fatal_token << " worker=" << worker_index);
}

// Run the fatal-output stress scenario in the child process.
//
// stderr_pipe_write becomes the child's stderr and abort_pipe_write receives one byte from the SIGABRT handler.
// The function never returns successfully: the expected path terminates the child through std::abort.
void run_child(int stderr_pipe_write, int abort_pipe_write)
{
  ::dup2(stderr_pipe_write, STDERR_FILENO);
  ::close(stderr_pipe_write);
  abort_signal_fd = abort_pipe_write;

  struct sigaction action;
  std::memset(&action, 0, sizeof(action));
  action.sa_handler = record_abort_signal;
  sigemptyset(&action.sa_mask);
  sigaction(SIGABRT, &action, nullptr);

  Debug(main_reached());
  Debug(libcw_do.set_ostream(&std::cerr, &debug_output_mutex));

  int const thread_count = concurrent_thread_count();
  utils::threading::StartingGate output_gate(thread_count);
  utils::threading::StartingGate fatal_gate(thread_count);
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  for (int index = 0; index < thread_count; ++index)
    workers.emplace_back(child_worker, index, std::ref(output_gate), std::ref(fatal_gate));

  for (std::thread& worker : workers)
    worker.join();

  std::cerr << "DoutFatal did not terminate the child process\n";
  std::_Exit(EXIT_FAILURE);
}

} // namespace

int main()
{
  int stderr_pipe[2];
  int abort_pipe[2];
  if (::pipe(stderr_pipe) == -1 || ::pipe(abort_pipe) == -1)
  {
    std::cerr << "pipe failed: " << std::strerror(errno) << '\n';
    return EXIT_FAILURE;
  }

  pid_t const child_pid = ::fork();
  if (child_pid == -1)
  {
    std::cerr << "fork failed: " << std::strerror(errno) << '\n';
    return EXIT_FAILURE;
  }

  if (child_pid == 0)
  {
    ::close(stderr_pipe[0]);
    ::close(abort_pipe[0]);
    run_child(stderr_pipe[1], abort_pipe[1]);
  }

  ::close(stderr_pipe[1]);
  ::close(abort_pipe[1]);

  int child_status;
  if (!wait_for_child(child_pid, child_status))
  {
    std::cerr << "waitpid failed: " << std::strerror(errno) << '\n';
    return EXIT_FAILURE;
  }

  std::string captured_stderr;
  std::string abort_markers;
  bool const read_stderr_ok = read_all_from_fd(stderr_pipe[0], captured_stderr);
  bool const read_abort_ok = read_all_from_fd(abort_pipe[0], abort_markers);
  ::close(stderr_pipe[0]);
  ::close(abort_pipe[0]);

  if (!read_stderr_ok || !read_abort_ok)
  {
    std::cerr << "failed to read child pipes\n";
    return EXIT_FAILURE;
  }

  if (abort_markers.size() != 1)
  {
    std::cerr << "expected exactly one SIGABRT marker, got " << abort_markers.size() << "\n"
              << "child stderr was:\n"
              << captured_stderr;
    return EXIT_FAILURE;
  }

  if (captured_stderr.find(fatal_token) == std::string::npos)
  {
    std::cerr << "fatal debug output token was not printed\nchild stderr was:\n" << captured_stderr;
    return EXIT_FAILURE;
  }

  if (!WIFSIGNALED(child_status) || WTERMSIG(child_status) != SIGABRT)
  {
    std::cerr << "expected child to exit due to signal SIGABRT; instead got status=" << child_status
              << "\nchild stderr was:\n"
              << captured_stderr;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
