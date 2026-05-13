// Exercise per-thread margins, color settings, and continued debug output.
// The test captures the output, strips ANSI SGR escape sequences for textual
// validation, and keeps the leading color sequence associated with each thread
// prefix to verify that every line from a thread uses the same color.
// Thread scheduling may interleave lines arbitrarily, so validation groups
// the captured output by the hexadecimal thread-id margin.

#include "cwd_sys.h"
#include "test_support.h"

#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int number_of_threads = 4;

std::mutex output_mutex;
std::atomic_int color_code{30};

struct parsed_line_ct {
  std::string color;
  std::string text;
};

// Strip ANSI SGR escape sequences from one line and return both the plain text
// and the first non-reset SGR sequence. Lines without an SGR color are accepted
// by the parser but rejected later for thread output. Unsupported escape forms
// are copied literally so that subsequent textual checks fail with useful input.
parsed_line_ct parse_line(std::string const& line)
{
  parsed_line_ct parsed;

  for (std::string::size_type pos = 0; pos < line.size();)
  {
    if (line[pos] == '\033' && pos + 1 < line.size() && line[pos + 1] == '[')
    {
      std::string::size_type end = line.find('m', pos + 2);
      if (end != std::string::npos)
      {
        std::string sgr = line.substr(pos, end - pos + 1);
        if (sgr != "\033[0m" && parsed.color.empty())
          parsed.color = sgr;
        pos = end + 1;
        continue;
      }
    }

    parsed.text.push_back(line[pos++]);
  }

  return parsed;
}

bool is_hex_prefix(std::string const& prefix)
{
  if (prefix.empty())
    return false;
  for (char c : prefix)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      return false;
  return true;
}

// Validate that a thread produced the exact tests/colors.cc message sequence.
// The prefix is passed only for diagnostics; messages must start with
// thread_main(), then ten loop iterations, where even iterations use the
// unfinished/indented/continued four-line sequence and odd iterations use the
// two-line complete continued-output sequence.
bool validate_thread_messages(std::string const& prefix, std::vector<std::string> const& messages)
{
  std::vector<std::string> expected;
  expected.push_back("thread_main()");
  for (int n = 0; n < 10; ++n)
  {
    std::ostringstream numbered_line;
    numbered_line << n << ". this is a single line.";
    expected.push_back(numbered_line.str());
    if (n % 2 == 0)
    {
      expected.push_back("This is a line <unfinished>");
      expected.push_back("    n is even!");
      expected.push_back("<continued> in two parts.");
    }
    else
      expected.push_back("This is a line in two parts.");
  }

  if (messages.size() != expected.size())
  {
    std::cerr << "Thread " << prefix << " produced " << messages.size()
              << " lines; expected " << expected.size() << "\n";
    return false;
  }

  for (std::size_t i = 0; i < expected.size(); ++i)
  {
    if (messages[i] != expected[i])
    {
      std::cerr << "Thread " << prefix << " line " << i << " mismatch: expected `"
                << expected[i] << "', got `" << messages[i] << "'\n";
      return false;
    }
  }

  return true;
}

void thread_main()
{
  std::ostringstream margin;
  margin << std::hex << std::this_thread::get_id() << ' ';
  Debug(libcw_do.margin().assign(margin.str().c_str(), margin.str().size()));

  std::ostringstream color_on;
  color_on << "\033[" << ++color_code << 'm';
  Debug(libcw_do.color_on().assign(color_on.str().c_str(), color_on.str().size()));
  Debug(libcw_do.color_off().assign("\033[0m", 4));

  Debug(libcw_do.on());
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

} // namespace

int main()
{
  std::ostringstream captured;

  Debug(check_configuration());
  Debug(libcw_do.set_ostream(&captured, &output_mutex));

  std::vector<std::thread> thread_pool;
  thread_pool.reserve(number_of_threads);
  for (int i = 0; i < number_of_threads; ++i)
    thread_pool.emplace_back(thread_main);

  for (std::thread& thread : thread_pool)
    thread.join();

  std::map<std::string, std::string> color_by_prefix;
  std::map<std::string, std::vector<std::string>> messages_by_prefix;

  std::istringstream lines(captured.str());
  std::string raw_line;
  while (std::getline(lines, raw_line))
  {
    parsed_line_ct parsed = parse_line(raw_line);
    std::string::size_type first_space = parsed.text.find(' ');
    if (first_space == std::string::npos)
    {
      std::cerr << "Line has no thread prefix separator: `" << parsed.text << "'\n";
      return EXIT_FAILURE;
    }

    std::string prefix = parsed.text.substr(0, first_space);
    if (!is_hex_prefix(prefix))
    {
      std::cerr << "Line has non-hex thread prefix: `" << parsed.text << "'\n";
      return EXIT_FAILURE;
    }

    if (parsed.color.empty())
    {
      std::cerr << "Line for thread " << prefix << " has no color: `" << raw_line << "'\n";
      return EXIT_FAILURE;
    }

    auto color_result = color_by_prefix.emplace(prefix, parsed.color);
    if (!color_result.second && color_result.first->second != parsed.color)
    {
      std::cerr << "Thread " << prefix << " changed color from `" << color_result.first->second
                << "' to `" << parsed.color << "'\n";
      return EXIT_FAILURE;
    }

    std::string const marker = " NOTICE  : ";
    if (parsed.text.compare(first_space, marker.size(), marker) != 0)
    {
      std::cerr << "Line for thread " << prefix << " lacks notice marker: `" << parsed.text << "'\n";
      return EXIT_FAILURE;
    }

    messages_by_prefix[prefix].push_back(parsed.text.substr(first_space + marker.size()));
  }

  if (messages_by_prefix.size() != number_of_threads)
  {
    std::cerr << "Captured output for " << messages_by_prefix.size() << " threads; expected "
              << number_of_threads << "\n";
    return EXIT_FAILURE;
  }

  for (auto const& entry : messages_by_prefix)
    if (!validate_thread_messages(entry.first, entry.second))
      return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
