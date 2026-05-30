#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace libcwd_ctest {

// ---------------------------------------------------------------------------
// PendingStreamBuf
//
// A streambuf with two internal buffers:
//   - pending  : receives characters written through the ostream interface
//   - output   : the committed buffer; pending is appended here on sync()
//
// Characters written via overflow() accumulate in `pending_`.
// On sync() (i.e. flush), pending_ is appended to output_ and cleared.
// ---------------------------------------------------------------------------
class PendingStreamBuf : public std::streambuf
{
 public:
  // Append s directly to the output buffer, bypassing pending.
  void write_direct(std::string_view s)
  {
    output_ << s;
  }

  // ostream that writes directly into the output buffer.
  std::ostream& direct_ostream() { return output_; }

  // istream over the committed output buffer.
  // Note: seekg(0) is called each time so the caller always reads
  // from the beginning of whatever has been committed so far.
  std::istream& input_istream()
  {
    // Rebuild the read-side from the current output content.
    input_.str(output_.str());
    input_.clear();          // clear any prior eof/fail bits
    return input_;
  }

  // Current contents of the pending (unflushed) buffer.
  std::string pending() const { return pending_.str(); }

  // Current contents of the committed output buffer.
  std::string output() const { return output_.str(); }

 protected:
  // ---------------------------------------------------------------------------
  // streambuf virtuals
  // ---------------------------------------------------------------------------

  // Called when the put-area is full (we use unbuffered mode: no put-area,
  // so every character comes here individually).
  int_type overflow(int_type ch) override
  {
    if (ch != traits_type::eof())
        pending_.put(static_cast<char>(ch));
    return ch;
  }

  // Called on flush / std::endl / explicit sync().
  // Appends pending_ to output_ and resets pending_.
  int sync() override
  {
    output_ << pending_.str();
    pending_.str({});
    pending_.clear();
    return 0;              // 0 = success
  }

 private:
  std::ostringstream pending_;
  std::ostringstream output_;
  std::istringstream input_;
};


// ---------------------------------------------------------------------------
// PendingStream
//
// An std::ostream whose normal write path goes to a "pending" buffer.
// Flushing (sync / std::flush / std::endl) commits pending → output.
//
// Extra API:
//   direct_out()  → std::ostream& that writes straight into output
//   input()       → std::istream& over the committed output buffer
// ---------------------------------------------------------------------------
class PendingStream : public std::ostream
{
 public:
  PendingStream()
      : std::ostream(&buf_)   // wire our streambuf into the base ostream
  {}

  // Not copyable (streams never are), but moveable if needed.
  PendingStream(const PendingStream&)            = delete;
  PendingStream& operator=(const PendingStream&) = delete;

  // -----------------------------------------------------------------------
  // Extra accessors
  // -----------------------------------------------------------------------

  // Returns an ostream that bypasses the pending buffer and writes
  // directly into the committed output buffer.
  std::ostream& direct_out() { return buf_.direct_ostream(); }

  // Returns an istream over the committed output buffer.
  // The stream is rewound each call so reads always start from position 0.
  std::istream& input() { return buf_.input_istream(); }

  // Convenience: peek at raw buffer strings without going through streams.
  std::string pending_str() const { return buf_.pending(); }
  std::string output_str()  const { return buf_.output(); }

 private:
  PendingStreamBuf buf_;
};

} // namespace libcwd_ctest
