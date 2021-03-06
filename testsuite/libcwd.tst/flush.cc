#include "sys.h"
#include <unistd.h>
#include <libcwd/debug.h>
#include <iostream>

namespace libcwd {
  namespace channels {
    namespace dc {
      channel_ct generate("GENERATE");
    }
  }
}

void generate_tables(void)
{
#ifndef THREADTEST
  ssize_t __attribute__((unused)) res = write(1, "<sleeping>", 10);
#else
  Dout( dc::always|noprefix_cf|nonewline_cf, "<sleeping>");
#endif
  usleep(10000);
  Dout( dc::generate, "Inside generate_tables()" );
  std::cout << std::flush;
#ifndef THREADTEST
  res = write(1, "<sleeping>", 10);
#else
  Dout( dc::always|noprefix_cf|nonewline_cf, "<sleeping>");
#endif
  usleep(10000);
  Dout( dc::generate, "Leaving generate_tables()" );
  std::cout << std::flush;
  return;
}

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );
#ifndef THREADTEST
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
#if CWDEBUG_ALLOC && !defined(THREADTEST)
  libcwd::make_all_allocations_invisible_except(NULL);
#endif

  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  // Turn debug object on
  Debug( libcw_do.on() );

  //Debug( dc::io.off() );
  Dout( dc::notice|flush_cf|continued_cf, "Generating tables part1... " );
  generate_tables();
  Dout( dc::continued, "part2... " );
  generate_tables();
  Dout( dc::finish, "done" );

  Debug( libcw_do.off() );

  EXIT(0);
}
