#include <libcw/sysd.h>			// This must be the first header file
#include <libcw/debug.h>

int main(void)
{
  Debug( dc::notice.on() );		// Turn on the NOTICE Debug Channel
  Debug( libcw_do.on() );		// Turn the default Debug Object on

  Dout( dc::notice, "Hello World" );

  return 0;
}

