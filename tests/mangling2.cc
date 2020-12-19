#include "sys.h"
#include "debug.h"
#include "libcwd/type_info.h"
#include <iostream>

// Creating evio::AcceptedSocket<MyAcceptedSocketDecoder, OutputStream>

struct MyAcceptedSocketDecoder {};

namespace evio {

struct OutputStream {};

template<typename T1, typename T2>
struct AcceptedSocket
{
};

template<typename T>
void create(T&)
{
  std::cout << '"' << typeid(T).name() << '"' << std::endl;
  std::string str = libcwd::type_info_of<T const&>().demangled_name();
  std::cout << "Demangled: \"" << str << '"' << std::endl;
  LIBCWD_ASSERT(str == "evio::AcceptedSocket<MyAcceptedSocketDecoder, evio::OutputStream> const&");
}

} // namespace evio

evio::AcceptedSocket<MyAcceptedSocketDecoder, evio::OutputStream> as;

// Template instantiation
int main()
{
  evio::create(as);
  // This is what happens internally:
  //libcwd::_private_::extract_exact_name("22libcwd_type_info_exactIN4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS0_12OutputStreamEEEE",
  //                                      "N4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS_12OutputStreamEEE");
}
