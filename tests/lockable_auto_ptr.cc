// Verify lockable_auto_ptr ownership transfer and locking behavior under CTest.
//
// This covers the legacy testsuite/libcwd.tst/lockable_auto_ptr.cc scenarios:
// unlocked copy construction, unlocked derived-to-base copy construction,
// unlocked derived-to-base assignment, locked copy construction, locked
// derived-to-base copy construction, and locked derived-to-base assignment.
// Each scenario also checks that exactly one owner deletes the tracked object
// when the participating lockable_auto_ptr instances leave scope.

#include "cwd_sys.h"
#include "test_support.h"
#include <libcwd/lockable_auto_ptr.h>

#include <cstdlib>
#include <iostream>

namespace {

// Base type used for derived-to-base lockable_auto_ptr conversions.
//
// The virtual destructor keeps the conversion tests well-defined when ownership
// ends in a lockable_auto_ptr<tracked_base>, and it records deletion so each
// scenario can verify that ownership was neither lost nor duplicated.
class tracked_base
{
 public:
  static int live_count;
  static int destroyed_count;

  tracked_base() { ++live_count; }
  tracked_base(tracked_base const&) = delete;
  tracked_base& operator=(tracked_base const&) = delete;
  virtual ~tracked_base()
  {
    --live_count;
    ++destroyed_count;
  }
};

int tracked_base::live_count;
int tracked_base::destroyed_count;

// Derived type used to exercise converting constructors and assignments.
class tracked_derived : public tracked_base
{
};

// Reset global lifetime counters before a scenario starts.
//
// The caller must ensure no tracked object is alive; otherwise the following
// deletion count would no longer identify the current scenario.
void reset_tracking()
{
  tracked_base::live_count = 0;
  tracked_base::destroyed_count = 0;
}

// Check pointer value, debug ownership state, and strict-owner state for ptr.
//
// Returns false after printing a diagnostic when ptr does not point at expected,
// when ownership differs from expect_owner, or when a current owner has a
// different strict-owner state from expect_strict_owner.
template <class PointerType, class RawType>
bool expect_state(char const* name, libcwd::lockable_auto_ptr<PointerType> const& ptr, RawType const* expected,
                  bool expect_owner, bool expect_strict_owner)
{
  bool success = true;
  if (ptr.get() != expected)
  {
    std::cerr << name << ".get() was " << ptr.get() << ", expected " << expected << '\n';
    success = false;
  }
  if (ptr.is_owner() != expect_owner)
  {
    std::cerr << name << ".is_owner() was " << ptr.is_owner() << ", expected " << expect_owner << '\n';
    success = false;
  }
  if (expect_owner && ptr.strict_owner() != expect_strict_owner)
  {
    std::cerr << name << ".strict_owner() was " << ptr.strict_owner() << ", expected " << expect_strict_owner << '\n';
    success = false;
  }
  return success;
}

// Check that the completed scenario deleted exactly one tracked object.
//
// Returns false after printing the observed live and destroyed counts when the
// owner failed to delete the object, deleted it more than once, or leaked it.
bool expect_single_delete(char const* scenario)
{
  if (tracked_base::live_count != 0 || tracked_base::destroyed_count != 1)
  {
    std::cerr << scenario << ": live_count=" << tracked_base::live_count
              << ", destroyed_count=" << tracked_base::destroyed_count
              << "; expected live_count=0, destroyed_count=1\n";
    return false;
  }
  return true;
}

// Exercise copy construction from unlocked owners, including a derived-to-base copy.
//
// Ownership should move to each new unlocked copy while all participating smart
// pointers continue to store the same raw pointer value.
bool test_unlocked_copy_construction()
{
  reset_tracking();
  bool success = true;
  {
    tracked_derived* object = new tracked_derived;
    libcwd::lockable_auto_ptr<tracked_derived> ap1(object);

    success = expect_state("ap1", ap1, object, true, false) && success;

    libcwd::lockable_auto_ptr<tracked_derived> ap2(ap1);

    success = expect_state("ap1", ap1, object, false, false) && success;
    success = expect_state("ap2", ap2, object, true, false) && success;

    libcwd::lockable_auto_ptr<tracked_base> bp1(ap2);

    success = expect_state("ap2", ap2, object, false, false) && success;
    success = expect_state("bp1", bp1, object, true, false) && success;
  }
  return expect_single_delete("unlocked copy construction") && success;
}

// Exercise assignment from an unlocked derived owner to a base pointer.
//
// The assignment should move ownership to the destination, keep the source's raw
// pointer observable, and delete the object through the destination at scope exit.
bool test_unlocked_assignment()
{
  reset_tracking();
  bool success = true;
  {
    tracked_derived* object = new tracked_derived;
    libcwd::lockable_auto_ptr<tracked_derived> ap3(object);
    libcwd::lockable_auto_ptr<tracked_base> bp2;

    bp2 = ap3;

    success = expect_state("ap3", ap3, object, false, false) && success;
    success = expect_state("bp2", bp2, object, true, false) && success;
  }
  return expect_single_delete("unlocked assignment") && success;
}

// Exercise copy construction from a locked owner and then from its non-owner copy.
//
// A locked owner should remain the strict owner after copying, and further copies
// from a non-owner should observe the pointer without acquiring ownership.
bool test_locked_copy_construction()
{
  reset_tracking();
  bool success = true;
  {
    tracked_derived* object = new tracked_derived;
    libcwd::lockable_auto_ptr<tracked_derived> ap4(object);
    ap4.lock();

    success = expect_state("ap4", ap4, object, true, true) && success;

    libcwd::lockable_auto_ptr<tracked_derived> ap5(ap4);

    success = expect_state("ap4", ap4, object, true, true) && success;
    success = expect_state("ap5", ap5, object, false, false) && success;

    libcwd::lockable_auto_ptr<tracked_base> bp3(ap5);

    success = expect_state("ap5", ap5, object, false, false) && success;
    success = expect_state("bp3", bp3, object, false, false) && success;
  }
  return expect_single_delete("locked copy construction") && success;
}

// Exercise assignment from a locked derived owner to a base pointer.
//
// The locked source should keep strict ownership while the assignment destination
// merely observes the same raw pointer and performs no deletion at scope exit.
bool test_locked_assignment()
{
  reset_tracking();
  bool success = true;
  {
    tracked_derived* object = new tracked_derived;
    libcwd::lockable_auto_ptr<tracked_derived> ap6(object);
    ap6.lock();
    libcwd::lockable_auto_ptr<tracked_base> bp4;

    bp4 = ap6;

    success = expect_state("ap6", ap6, object, true, true) && success;
    success = expect_state("bp4", bp4, object, false, false) && success;
  }
  return expect_single_delete("locked assignment") && success;
}

} // namespace

int main()
{
  Debug(main_reached());

  bool success = true;
  success = test_unlocked_copy_construction() && success;
  success = test_unlocked_assignment() && success;
  success = test_locked_copy_construction() && success;
  success = test_locked_assignment() && success;

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
