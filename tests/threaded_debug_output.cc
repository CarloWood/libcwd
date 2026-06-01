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
#define DoutB(cntrl, data) LibcwDout(::libcwd::channels, extra_do, cntrl, data)

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
  DoutB(dc::notice, "Entering fB()");
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

  auto&& what = [](int w, int b) -> char const* { return (w & b) ? " flushed" : ""; };
  for (int wA = 0; wA < 8; ++wA)
    for (int wB = 0; wB < 8; ++wB)
    {
      Dout(dc::notice|conf_flush_cf(wA & 1)|continued_cf, "Calling fA(): " << start_nestingA() << fA() << stop_nestingA() << what(wA, 1) << ' ');
      DoutB(dc::notice|conf_flush_cf(wB & 1)|continued_cf, "Calling fB(): " << start_nestingB() << fB() << stop_nestingB() << what(wB, 1) << ' ');
      Dout(dc::continued|conf_flush_cf(wA & 2), "cont" << what(wA, 2));
      DoutB(dc::continued|conf_flush_cf(wB & 2), "cont" << what(wB, 2));
      Dout(dc::finish|conf_flush_cf(wA & 4), " finish" << what(wA, 4));
      DoutB(dc::finish|conf_flush_cf(wB & 4), " finish" << what(wB, 4));
    }
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

// Check the continued-output suffix after the nested fA/fB return value.
//
// A complete line must contain `cont' followed by `finish', with each of the
// three flush points optionally contributing the literal word `flushed'. An
// unfinished line may not contain `finish'; it is valid only when an earlier
// flush point made the unfinished fragment visible before the final finish.
bool return_value_suffix_is_valid(std::string const& suffix, bool end_on_unfinished)
{
  if (!end_on_unfinished)
  {
    return suffix == "cont finish" || suffix == "flushed cont finish" || suffix == "cont flushed finish" ||
           suffix == "flushed cont flushed finish" || suffix == "cont finish flushed" ||
           suffix == "flushed cont finish flushed" || suffix == "cont flushed finish flushed" ||
           suffix == "flushed cont flushed finish flushed";
  }

  return suffix == "cont flushed" || suffix == "flushed cont flushed" || suffix == "flushed cont" ||
         suffix == "flushed";
}

// Check second/third continued-output suffix.
//
// For example,
// T13 NOTICE  M13A             Calling fA(): [<unfinished>
// T13 NOTICE  ****M13A                 Entering fA()
// T13 NOTICE  M13A             <continued> fA return value] flushed <unfinished>
// T13 NOTICE  M13A             <continued> cont flushed<unfinished>                 <== second time the line is `<continued>`.
// T13 NOTICE  M13A             <continued>  finish flushed                          <== third time the line is `<continued>`.
//
// A complete line must start with `cont` followed by `finish', with each of the
// two flush points optionally contributing the literal word `flushed'. An
// unfinished line may not contain `finish'.
//
bool continued_text_is_valid(std::string const& text, bool end_on_unfinished)
{
  if (!end_on_unfinished)
  {
    // Second <continued> (starts with `cont`) that is finished.
    return text == "cont finish" ||
           text == "cont finish flushed" ||
           text == "cont flushed finish" ||
           text == "cont flushed finish flushed" ||
    // Third <continued>.
           text == " finish" || text == " finish flushed";
  }

  // Second <continued> that itself is unfinished: a `cont` that is optionally flushed.
  //     <continued> cont flushed<unfinished>
  //     <continued> cont<unfinished>
  return text == "cont flushed" || text == "cont";
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

  if (parsed.text.find("return value") != std::string::npos)
  {
    std::string const expected_prefix = std::string("f") + expected_marker_suffix + " return value] ";
    if (parsed.text.compare(0, expected_prefix.size(), expected_prefix) != 0)
    {
      std::cerr << stream_name << " line has return-value text for the wrong debug object or in the wrong form; "
                << "expected prefix `" << expected_prefix << "': `" << raw_line << "'\n";
      return false;
    }

    std::string const suffix = parsed.text.substr(expected_prefix.size());
    if (!return_value_suffix_is_valid(suffix, parsed.end_on_unfinished))
    {
      std::cerr << stream_name << " line has invalid continued-output suffix `" << suffix << "'"
                << (parsed.end_on_unfinished ? " before <unfinished>" : "") << ": `" << raw_line << "'\n";
      return false;
    }
  }
  else if (parsed.text.find("Calling") != std::string::npos)
  {
    std::string const expected_text = std::string("Calling f") + expected_marker_suffix + "(): [";
    if (parsed.text != expected_text)
    {
      std::cerr << stream_name << " line has `Calling' text for the wrong debug object or in the wrong form; "
                << "expected text `" << expected_text << "' got `" << parsed.text << "' in line: `" << raw_line << "'\n";
      return false;
    }
    else if (!parsed.end_on_unfinished)
    {
      std::cerr << stream_name << " line has `Calling' text but isn't ending on <unfinished>: `" << raw_line << "'\n";
      return false;
    }
  }
  else if (parsed.text.find("Entering") != std::string::npos)
  {
    std::string const expected_text = std::string("Entering f") + expected_marker_suffix + "()";
    if (parsed.text != expected_text)
    {
      std::cerr << stream_name << " line has `Entering' text for the wrong debug object or in the wrong form; "
                << "expected text `" << expected_text << "' got `" << parsed.text << "' in line: `" << raw_line << "'\n";
      return false;
    }
  }
  else if (!parsed.starts_with_continued)
  {
    std::cerr << stream_name << " line expected to start with <continued>, but isn't: `" << raw_line << "'\n";
    return false;
  }
  else if (!continued_text_is_valid(parsed.text, parsed.end_on_unfinished))
  {
    std::cerr << stream_name << " line has invalid continued-output format `" << parsed.text << "'"
              << (parsed.end_on_unfinished ? " before <unfinished>" : "") << ": `" << raw_line << "'\n";
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

  return EXIT_SUCCESS;
}
