#pragma once

#include "Gate.h"

namespace utils::threading
{

// Block threads until `stalls' threads are ready.
//
class StartingGate
{
 private:
  std::atomic_int m_stalls;
  Gate m_gate;

 public:
  StartingGate(int stalls) : m_stalls(stalls) { }

  void wait()
  {
    if (m_stalls-- == 1)
      m_gate.open();
    m_gate.wait();
  }
};

} // namespace utils::threading
