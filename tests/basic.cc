#include <libcw/sys.h>
#include <libcw/debug.h>

int main(void)
{
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  int i = 1;
  Dout(dc::notice, "Basic Test.");
  Dout(dc::warning, "This should not be printed." << ++i);
  Debug( dc::warning.on() );
  Dout(dc::warning, "This should be a one: " << i);

  exit(0);
}
