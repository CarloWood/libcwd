#include <libcw/sysd.h>
#include <fstream>
#include <libcw/debug.h>

int main(void)
{
  Debug( dc::notice.on() );
  Debug( libcw_do.on() );

#ifdef CWDEBUG
  ofstream file;
  file.open("log");
#endif

  // Set the ostream related with libcw_do to `file':  
  Debug( libcw_do.set_ostream(&file) );

  Dout( dc::notice, "Hippopotamus are heavy" );

  return 0;
}
