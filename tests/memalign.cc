#include <libcwd/sys.h>
#include <libcwd/debug.h>
#include <cstdlib>	// posix_memalign, aligned_alloc.
#include <malloc.h>	// memalign
#include <stdio.h>	// perror

int main()
{
  Debug(check_configuration());
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );

  void* ptr;
#if 0
  Dout(dc::notice|continued_cf, "memalign(128, 3302) = ");
  ptr = memalign(128, 3302);
  Dout(dc::finish|cond_error_cf(ptr == NULL), ptr);
#elif 0
  Dout(dc::notice|continued_cf, "valloc(3302) = ");
  ptr = valloc(3302);
  Dout(dc::finish|cond_error_cf(ptr == NULL), ptr);
#elif 0
  Dout(dc::notice|continued_cf, "posix_memalign(&ptr, 128, 3302) = ");
  int res = posix_memalign(&ptr, 128, 3302);
  Dout(dc::finish|cond_error_cf(res != 0), res);
  if (res != 0)
    perror("posix_memalign");
  Dout(dc::notice, "ptr == " << ptr);
#else
  Dout(dc::notice|continued_cf, "aligned_alloc(128, 3302) = ");
  ptr = aligned_alloc(128, 3302);
  Dout(dc::finish|cond_error_cf(ptr == NULL), ptr);
#endif
  Dout(dc::notice|continued_cf, "Calling free(" << ptr << ")... ");
  free(ptr);
  Dout(dc::finish, "done");
}
