#include "sys.h"
#include <libcw/debug.h>

int main(void)
{
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  Dout(dc::notice, "Basic Test.");

  exit(0);
}
