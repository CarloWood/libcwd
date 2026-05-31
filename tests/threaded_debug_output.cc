// Start several threads at the same gate and let each thread emit one libcwd
// debug line. This is an initial CTest-backed concurrency smoke test: it does
// not inspect the output, but it exercises the multi-threaded debug-output path
// with a real ostream lock installed before the worker threads run.

#include "cwd_sys.h"
#include "test_support.h"

#include "StartingGate.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr int number_of_threads = 16;

std::mutex output_mutex;

// Prepare this worker thread for libcwd debug output, using thread_index as the
// two-digit debug margin for this thread. Wait until all workers have reached
// the same point, and then write the single line that the test is meant to
// exercise. The gate keeps thread creation latency out of the main work so the
// Dout calls contend for the shared debug-output path as much as the scheduler
// permits; failures are reported by libcwd itself or by thread joins.
void thread_main(int thread_index, utils::threading::StartingGate& starting_gate)
{
  char margin[5];
  std::snprintf(margin, sizeof(margin), "T%02d ", thread_index);
  Debug(libcw_do.margin().assign(margin, sizeof(margin) - 1));
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  starting_gate.wait();

  Dout(dc::notice, "Hello");
}

} // namespace

int main()
{
  Debug(main_reached());
  Debug(libcw_do.set_ostream(&std::cerr, &output_mutex));

  utils::threading::StartingGate starting_gate(number_of_threads);
  std::vector<std::thread> thread_pool;
  thread_pool.reserve(number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
    thread_pool.emplace_back(thread_main, i, std::ref(starting_gate));

  for (std::thread& thread : thread_pool)
    thread.join();

  return EXIT_SUCCESS;
}
