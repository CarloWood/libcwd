#include "sys.h"
#include <libcwd/debug.h>
#include <iostream>

void S_destroy(void* tsd_ptr) throw()
{
  delete (int*)tsd_ptr;
}

int main()
{
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );
  //Debug( dc::malloc.on() );

  // This tests if the warning channel is turned on.
  Dout(dc::warning, "Ok");

  pthread_key_t S_key;
  pthread_key_create(&S_key, S_destroy);
  int* p = new int;
  *p = 123454321;
  pthread_setspecific(S_key, p);
  int* q = (int*)pthread_getspecific(S_key);
  std::cout << "q = " << q << "; *q = " << *q << '\n';
  int* q2 = (int*)pthread_getspecific(S_key);
  assert(q == q2);

  exit(0);
}
