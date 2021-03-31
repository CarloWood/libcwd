#include "sys.h"
#include "debug.h"
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <atomic>

constexpr int number_of_threads = 4;

std::mutex cout_mutex;
std::atomic_int color_code{30};

void thread_main()
{
  // Set the margin equal to the thread ID.
  std::ostringstream margin;
  margin << std::hex << std::this_thread::get_id() << ' ';
  Debug(libcw_do.margin().assign(margin.str().c_str(), margin.str().size()));

  // Set a color code.
  std::ostringstream color_on;
  color_on << "\e[" << ++color_code << "m";
  Debug(libcw_do.color_on().assign(color_on.str().c_str(), color_on.str().size()));
  Debug(libcw_do.color_off().assign("\e[0m", 4));

  // Turn on debug output and all channels.
  Debug(libcw_do.on() );
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on(););

  Dout(dc::notice, "thread_main()");

  for (int n = 0; n < 10; ++n)
  {
    Dout(dc::notice, n << ". this is a single line.");
    Dout(dc::notice|continued_cf, "This is a line ");
    if (n % 2 == 0)
      Dout(dc::notice, "n is even!");
    Dout(dc::finish, "in two parts.");
  }
}

int main()
{
  Debug(check_configuration());
#if CWDEBUG_ALLOC
  libcwd::make_all_allocations_invisible_except(NULL);
#endif
  Debug(libcw_do.set_ostream(&std::cout, &cout_mutex));

  // Set the margin equal to the thread ID.
  std::ostringstream margin;
  margin << std::hex << std::this_thread::get_id() << ' ';
  Debug(libcw_do.margin().assign(margin.str().c_str(), margin.str().size()));

  // Turn on debug output and all channels.
  Debug(libcw_do.on() );
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on(););
  // Print the channels.
  Debug(list_channels_on(libcw_do));

  {
    // Create number_of_threads threads.
    std::vector<std::thread> thread_pool;
    thread_pool.reserve(number_of_threads);
    for(int i = 0; i < number_of_threads; ++i)
      thread_pool.emplace_back(thread_main);

    // Join with all the threads.
    for(int i = 0; i < number_of_threads; ++i)
      thread_pool[i].join();
  }

  Dout(dc::notice, "Leaving main thread.");
}
