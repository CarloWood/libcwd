#pragma once

#include <string>

// This file declares the debug::init and debug::init_thread functions
// of the pre- libcwd-2 cwds git submodule.

namespace libcwd::init_functions {

enum thread_init_t
{
  thread_init_default,
  from_rcfile,
  copy_from_main,
  debug_off
};

void init();
void init_thread(std::string thread_name = "", thread_init_t thread_init = thread_init_default);

} // libcwd::init_functions

namespace libcwd {
using namespace init_functions;
} // namespace libcwd
