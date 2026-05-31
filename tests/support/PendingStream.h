#pragma once

#include "threadsafe/AIMutex.h"
#include "libcwd/class_debug.h"

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

  // istream that reads directly from the output buffer.
  std::istream& direct_istream() { return output_; }

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
  std::stringstream output_;
};


// ---------------------------------------------------------------------------
// PendingStream
//
// An std::ostream whose normal write path goes to a "pending" buffer.
// Flushing (sync / std::flush / std::endl) commits pending → output.
//
// Extra API:
//   direct_ostream()  → std::ostream& that writes straight into the "output" buffer.
//   direct_istream()  → std::istream& that reads straight from the "output" buffer.
// ---------------------------------------------------------------------------
class PendingStream : public std::ostream
{
 public:
  PendingStream(libcwd::debug_ct& debug_object)
      : std::ostream(&buf_)   // wire our streambuf into the base ostream
  {
    debug_object.set_ostream(this, &output_mutex_);
  }

  // Not copyable (streams never are), but moveable if needed.
  PendingStream(const PendingStream&)            = delete;
  PendingStream& operator=(const PendingStream&) = delete;

  // -----------------------------------------------------------------------
  // Extra accessors
  // -----------------------------------------------------------------------

  // Returns an ostream that bypasses the pending buffer and writes
  // directly into the committed output buffer.
  std::ostream& direct_ostream() { return buf_.direct_ostream(); }

  // Returns an istream over the committed output buffer.
  // The stream is rewound each call so reads always start from position 0.
  std::istream& direct_istream() { return buf_.direct_istream(); }

  // Convenience: peek at raw buffer strings without going through streams.
  std::string pending_str() const { return buf_.pending(); }
  std::string output_str()  const { return buf_.output(); }

  void sync()
  {
    std::lock_guard<AIMutex> lock(output_mutex_);
    buf_.pubsync();
  }

 private:
  PendingStreamBuf buf_;
  AIMutex output_mutex_;
};

} // namespace libcwd_ctest
