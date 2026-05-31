// Start several threads at the same gate and let each thread emit one libcwd
// debug line. This is an initial CTest-backed concurrency smoke test: it does
// not inspect the output, but it exercises the multi-threaded debug-output path
// with a real ostream lock installed before the worker threads run.

#include "cwd_sys.h"
#include "test_support.h"

#include "StartingGate.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int number_of_threads = 16;

std::mutex output_mutex;

libcwd::debug_ct extra_do;
std::mutex extra_output_mutex;
#define Dout2(cntrl, data) LibcwDout(::libcwd::channels, extra_do, cntrl, data)

char start_nestingA()
{
  libcwd::libcw_do.push_marker();
  libcwd::libcw_do.marker().prepend("****", 4);
  return '[';
}

char stop_nestingA()
{
  libcwd::libcw_do.pop_marker();
  return ']';
}

char start_nestingB()
{
  extra_do.push_marker();
  extra_do.marker().prepend("****", 4);
  return '[';
}

char stop_nestingB()
{
  extra_do.pop_marker();
  return ']';
}

std::string fA()
{
  Dout(dc::notice, "Entering fA()");
  return "fA return value";
}

std::string fB()
{
  Dout2(dc::notice, "Entering fB()");
  return "fB return value";
}

// Prepare this worker thread for libcwd debug output, using thread_index as the
// two-digit debug margin for this thread. Wait until all workers have reached
// the same point, and then write the single line that the test is meant to
// exercise. The gate keeps thread creation latency out of the main work so the
// Dout calls contend for the shared debug-output path as much as the scheduler
// permits; failures are reported by libcwd itself or by thread joins.
void thread_main(int thread_index, utils::threading::StartingGate& starting_gate)
{
  // Set margin on both debug objects.
  char buf[5];  // 'T', '[01]', '[0-9]', '[ AB]', '\0'
  std::snprintf(buf, sizeof(buf), "T%02d ", thread_index);
  Debug(libcw_do.margin().assign(buf, sizeof(buf) - 1));
  Debug(extra_do.margin().assign(buf, sizeof(buf) - 1));

  // Set marker on both debug objects.
  buf[3] = 'A';
  std::snprintf(buf, sizeof(buf), "M%02dA", thread_index);
  Debug(libcw_do.marker().assign(buf, sizeof(buf) - 1));
  buf[3] = 'B';
  Debug(extra_do.marker().assign(buf, sizeof(buf) - 1));

  // Set indentation on both debug objects to the same value as the thread
  // number encoded in the margin and marker so validation can detect state that
  // leaks between threads or debug objects.
  Debug(libcw_do.set_indent(thread_index));
  Debug(extra_do.set_indent(thread_index));

  // Turn on.
  Debug(libcw_do.on());
  Debug(extra_do.on());
  Debug(dc::notice.on());

  starting_gate.wait();

  Dout(dc::notice, "Calling fA(): " << start_nestingA() << fA() << stop_nestingA());
  Dout2(dc::notice, "Calling fB(): " << start_nestingB() << fB() << stop_nestingB());
}

} // namespace

struct parsed_line_ct
{
  std::string margin;
  std::string label;
  int stars;
  std::string marker;
  int indent;
  std::string text;
  bool end_on_unfinished;
  bool starts_with_continued;
};

std::ostream& operator<<(std::ostream& os, parsed_line_ct const& parsed_line)
{
  os << parsed_line.margin << parsed_line.label << parsed_line.marker << std::string(parsed_line.indent - parsed_line.stars, ' ')
     << std::string(parsed_line.stars, '<') << parsed_line.text;
  return os;
}

struct ParseCompare
{
  bool operator()(parsed_line_ct const& l1, parsed_line_ct const& l2)
  {
    if (l1.margin != l2.margin)
      return l1.margin < l2.margin;

    if (l1.marker != l2.marker)
      return l1.marker < l2.marker;

    if (l1.label != l2.label)
      return l1.label < l2.label;

    return l1.text < l2.text;
  }
};

parsed_line_ct parse_line(std::string const& line)
{
  parsed_line_ct parsed;

  // Parse the fixed prefix layout used by this test: a four-character margin,
  // libcwd's eight-character channel label, a four-character marker, then the
  // per-thread indentation and message text. The function throws on malformed
  // input so the CTest fails immediately if concurrent output corrupts a line or
  // if libcwd changes the expected prefix shape.
  constexpr std::size_t margin_size = 4;       // "T%02d "
  constexpr std::size_t label_size = 8;        // "NOTICE  "
  constexpr std::size_t marker_size = 4;       // "M%02d[AB]"
  constexpr std::size_t prefix_size = margin_size + label_size + marker_size;

  if (line.size() < prefix_size)
    throw std::runtime_error("debug output line is shorter than the expected prefix: `" + line + "'");

  parsed.margin = line.substr(0, margin_size);
  parsed.label = line.substr(margin_size, label_size);
  std::size_t stars_pos = margin_size + label_size;
  while (stars_pos < line.size() && line[stars_pos] == '*')
    ++stars_pos;
  parsed.stars = static_cast<int>(stars_pos - (margin_size + label_size));
  parsed.marker = line.substr(margin_size + label_size + parsed.stars, marker_size);

  std::size_t text_pos = prefix_size + parsed.stars;
  while (text_pos < line.size() && line[text_pos] == ' ')
    ++text_pos;
  parsed.indent = static_cast<int>(text_pos - (prefix_size + parsed.stars));
  parsed.text = line.substr(text_pos);

  std::string const continued_prefix = "<continued> ";
  parsed.starts_with_continued = parsed.text.compare(0, continued_prefix.size(), continued_prefix) == 0;
  if (parsed.starts_with_continued)
    parsed.text.erase(0, continued_prefix.size());

  std::string const unfinished_suffix = "<unfinished>";
  parsed.end_on_unfinished = parsed.text.size() >= unfinished_suffix.size() &&
                             parsed.text.compare(parsed.text.size() - unfinished_suffix.size(),
                                                 unfinished_suffix.size(), unfinished_suffix) == 0;
  if (parsed.end_on_unfinished)
  {
    parsed.text.erase(parsed.text.size() - unfinished_suffix.size());
    if (!parsed.text.empty() && parsed.text.back() == ' ')
      parsed.text.pop_back();
  }

  return parsed;
}

// Return the numeric value encoded by two decimal digits in text at offset.
//
// The caller supplies field_name and raw_line for diagnostics. This function
// throws when either character is not decimal because the caller cannot safely
// compare the malformed field with the parsed indentation value.
int parse_two_digits(std::string const& text, std::size_t offset, char const* field_name, std::string const& raw_line)
{
  if (text[offset] < '0' || text[offset] > '9' || text[offset + 1] < '0' || text[offset + 1] > '9')
    throw std::runtime_error(std::string(field_name) + " does not contain two decimal digits in line `" + raw_line + "'");

  return (text[offset] - '0') * 10 + text[offset + 1] - '0';
}

// Validate that one parsed debug-output line carries the per-thread state that
// threaded_debug_output assigned before the worker passed the starting gate.
// expected_marker_suffix is 'A' for libcw_do output and 'B' for extra_do output.
//
// Returns false after printing a diagnostic when the margin or marker shape is
// wrong, or when their encoded thread number does not match parsed.indent.
bool validate_thread_fields(parsed_line_ct const& parsed, char expected_marker_suffix, char const* stream_name,
                            std::string const& raw_line)
{
  if (parsed.margin.size() != 4 || parsed.margin[0] != 'T' || parsed.margin[3] != ' ')
  {
    std::cerr << stream_name << " line has malformed margin: `" << raw_line << "'\n";
    return false;
  }
  int const margin_value = parse_two_digits(parsed.margin, 1, "margin", raw_line);
  if (margin_value != parsed.indent - parsed.stars)
  {
    std::cerr << stream_name << " line margin value " << margin_value << " does not match indent minus stars "
              << parsed.indent << " - " << parsed.stars << ": `" << raw_line << "'\n";
    return false;
  }

  if (parsed.marker.size() != 4 || parsed.marker[0] != 'M' || parsed.marker[3] != expected_marker_suffix)
  {
    std::cerr << stream_name << " line has malformed marker, expected suffix `" << expected_marker_suffix << "': `"
              << raw_line << "'\n";
    return false;
  }
  int const marker_value = parse_two_digits(parsed.marker, 1, "marker", raw_line);
  if (marker_value != parsed.indent - parsed.stars)
  {
    std::cerr << stream_name << " line marker value " << marker_value << " does not match indent minus stars "
              << parsed.indent << " - " << parsed.stars << ": `" << raw_line << "'\n";
    return false;
  }

  return true;
}

int main()
{
  Debug(main_reached());

  libcwd_ctest::PendingStream captured(libcwd::libcw_do);
  libcwd_ctest::PendingStream extra_captured(extra_do);

  utils::threading::StartingGate starting_gate(number_of_threads);
  std::vector<std::thread> thread_pool;
  thread_pool.reserve(number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
    thread_pool.emplace_back(thread_main, i, std::ref(starting_gate));

  for (std::thread& thread : thread_pool)
    thread.join();

  captured.sync();
  extra_captured.sync();

  std::cout << "Captured output:\n" << captured.output_str() << std::endl;
  std::cout << "Extra captured output:\n" << extra_captured.output_str() << std::endl;

  std::vector<parsed_line_ct> output;
  std::vector<parsed_line_ct> extra_output;

  std::string raw_line;
  while (std::getline(captured.direct_istream(), raw_line))
  {
    parsed_line_ct parsed = parse_line(raw_line);
    if (!validate_thread_fields(parsed, 'A', "captured", raw_line))
      return EXIT_FAILURE;
    output.push_back(parsed);
  }
  while (std::getline(extra_captured.direct_istream(), raw_line))
  {
    parsed_line_ct parsed = parse_line(raw_line);
    if (!validate_thread_fields(parsed, 'B', "extra_captured", raw_line))
      return EXIT_FAILURE;
    extra_output.push_back(parsed);
  }

  std::sort(output.begin(), output.end(), ParseCompare{});
  std::sort(extra_output.begin(), extra_output.end(), ParseCompare{});

  std::cout << "Sorted output:\n";
  for (parsed_line_ct const& parsed_line : output)
    std::cout << parsed_line << '\n';
  std::cout << "Extra sorted output:\n";
  for (parsed_line_ct const& parsed_line : extra_output)
    std::cout << parsed_line << '\n';

  return EXIT_SUCCESS;
}
