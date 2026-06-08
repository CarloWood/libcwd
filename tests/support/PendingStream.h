#pragma once

#include "threadsafe/AIMutex.h"
#include "libcwd/class_debug.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

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
  PendingStreamBuf() : output_mutex_(nullptr), lock_violation_detected_(false) { }

  // Remember the mutex that must be locked by the current thread before this
  // stream buffer is touched through the std::ostream interface.
  //
  // Passing nullptr disables the check; callers normally pass the mutex that is
  // also supplied to libcwd's debug object so stream writes can verify that
  // libcwd took the expected ostream lock.
  void require_locked_by(AIMutex const* output_mutex)
  {
    output_mutex_ = output_mutex;
    lock_violation_detected_.store(false, std::memory_order_relaxed);
  }

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

  // Return whether any std::ostream write or flush reached this stream buffer
  // without the configured AIMutex being locked by the current thread.
  //
  // This is sticky after the first violation so callers can inspect it after a
  // multi-threaded scenario has completed.
  bool lock_violation_detected() const { return lock_violation_detected_.load(std::memory_order_relaxed); }

 protected:
  // ---------------------------------------------------------------------------
  // streambuf virtuals
  // ---------------------------------------------------------------------------

  // Called when the put-area is full (we use unbuffered mode: no put-area,
  // so every character comes here individually).
  int_type overflow(int_type ch) override
  {
    check_locked();
    if (ch != traits_type::eof())
        pending_.put(static_cast<char>(ch));
    return ch;
  }

  // Write a block of characters to the pending buffer.
  //
  // The entire block is accepted only after verifying that the current thread
  // owns the expected ostream mutex. Returning n preserves normal streambuf
  // semantics for successful writes; lock violations are recorded for the test
  // to report after the debug-output scenario finishes.
  std::streamsize xsputn(char const* s, std::streamsize n) override
  {
    check_locked();
    pending_.write(s, n);
    return n;
  }

  // Called on flush / std::endl / explicit sync().
  // Appends pending_ to output_ and resets pending_.
  int sync() override
  {
    check_locked();
    output_ << pending_.str();
    pending_.str({});
    pending_.clear();
    return 0;              // 0 = success
  }

 private:
  // Record that an ostream operation was performed without the configured
  // mutex being held by this thread.
  //
  // No exception is thrown from streambuf virtuals; tests can query the sticky
  // flag with lock_violation_detected() and print diagnostics from normal code.
  void check_locked()
  {
    if (output_mutex_ && !output_mutex_->is_self_locked())
      lock_violation_detected_.store(true, std::memory_order_relaxed);
  }

  std::ostringstream pending_;
  std::stringstream output_;
  AIMutex const* output_mutex_;
  std::atomic_bool lock_violation_detected_;
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
  PendingStream(libcwd::DebugObject& debug_object)
      : std::ostream(&buf_)   // wire our streambuf into the base ostream
  {
    buf_.require_locked_by(&output_mutex_);
    debug_object.set_ostream(static_cast<std::ostream*>(this), &output_mutex_);
  }

  // Not copyable (streams never are), but moveable if needed.
  PendingStream(const PendingStream&)            = delete;
  PendingStream& operator=(const PendingStream&) = delete;

  // Write test-side data through the pending ostream while holding the same
  // mutex that libcwd uses for this stream.
  //
  // This keeps legacy uses such as `captured << std::endl` valid without
  // weakening the streambuf check that every ostream operation must happen
  // while output_mutex_ is locked. The value may be a normal printable object
  // or an ostream manipulator such as std::flush or std::endl.
  template<typename T>
  PendingStream& operator<<(T&& value)
  {
    std::lock_guard<AIMutex> lock(output_mutex_);
    static_cast<std::ostream&>(*this) << std::forward<T>(value);
    return *this;
  }

  PendingStream& operator<<(std::ostream& (*manipulator)(std::ostream&))
  {
    std::lock_guard<AIMutex> lock(output_mutex_);
    manipulator(static_cast<std::ostream&>(*this));
    return *this;
  }

  PendingStream& operator<<(std::ios& (*manipulator)(std::ios&))
  {
    std::lock_guard<AIMutex> lock(output_mutex_);
    manipulator(static_cast<std::ostream&>(*this));
    return *this;
  }

  PendingStream& operator<<(std::ios_base& (*manipulator)(std::ios_base&))
  {
    std::lock_guard<AIMutex> lock(output_mutex_);
    manipulator(static_cast<std::ostream&>(*this));
    return *this;
  }

  // -----------------------------------------------------------------------
  // Extra accessors
  // -----------------------------------------------------------------------

  // Returns an ostream that bypasses the pending buffer and writes
  // directly into the committed output buffer.
  std::ostream& direct_ostream() { return buf_.direct_ostream(); }

  // Returns the stream that stores the committed output and can be read from its
  // current get position. Callers that need to reread data should seek it
  // explicitly or use output_str() to construct a fresh input stream.
  std::istream& direct_istream() { return buf_.direct_istream(); }

  // Convenience: peek at raw buffer strings without going through streams.
  std::string pending_str() const { return buf_.pending(); }
  std::string output_str()  const { return buf_.output(); }

  // Return whether the stream buffer observed any std::ostream operation that
  // did not hold this PendingStream's AIMutex.
  //
  // The flag covers writes and flushes performed through the ostream installed
  // on the libcwd debug object; helper accessors that bypass that ostream are
  // intentionally outside the checked path.
  bool lock_violation_detected() const { return buf_.lock_violation_detected(); }

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
