#include <libcw/sysd.h>
#include <libcw/debug.h>
#include <libcw/type_info.h>

#define builtin_return_address(addr) ((char*)__builtin_return_address(addr) + libcw::debug::builtin_return_address_offset)

template<typename T>
  class A {
    int i;
  public:
    A(void) {};
  };

template<typename T>
  class B {
    int i;
  public:
    B(void) {};
    ~B(void);
  };

template<typename T1, typename T2>
  class C : public A<T1>, public B<T2> {
    int i;
  public:
    C(void) {};
  };

template<typename T>
B<T>::~B(void)
{
  Dout(dc::notice, "Calling the destructor of " <<
                   libcw::debug::type_info_of(*this).demangled_name() << " (this == " << this << ")");
  libcw::debug::alloc_ct const* alloc = libcw::debug::find_alloc(this);
  if (sizeof(*this) != alloc->size())
  {
    Debug( dc::malloc.off() );
    Debug( libcw_do.set_marker(": | ") );
    Dout(dc::notice, "This is a base class of an object starting at " << alloc->start());
    Dout(dc::notice, "The type of the pointer to the allocated object is " <<
                     alloc->type_info().demangled_name());
    Debug( libcw_do.set_marker(": ` ") );
    Dout(dc::notice, "The destructor was called from " << location_ct(builtin_return_address(0)));
    Debug( dc::malloc.on() );
    Debug( libcw_do.set_marker(": ") );
  }
}

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );
  Debug( dc::malloc.on() );

  B<int>* b = new B<int>;				// line 55
  AllocTag(b, "object `b'");
  Dout(dc::notice, "b is " << b);

  C<double, B<char> >* c = new C<double, B<char> >;	// line 59
  AllocTag(c, "object `c'");
  Dout(dc::notice, "c is " << c);

  delete b;
  delete c;						// line 64

  return 0;
}
