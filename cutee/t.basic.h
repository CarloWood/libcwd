#ifndef T_BASIC_H
#define T_BASIC_H

#include <iostream>
#include <streambuf>
#include <iomanip>
#include <sstream>
#include <string>
#include "cutee.h"
#include "libcwd/config.h"
#include "libcwd/debug.h"

// this is used to easily redirect cerr/cout to another ostream 
struct redirect {
private:
  std::streambuf* pSb;

public:
  redirect(std::ostream& os) { pSb = std::cerr.rdbuf(os.rdbuf()); }
  ~redirect() { std::cerr.rdbuf(pSb); }
};

bool PatternCompareFunc(std::string const& l, std::string const& r)
{
  // You can implement here a simple wildcard comparison func or use a 
  // more powerful regex library or whatever you need.
  return l == r;
}

struct TEST_CLASS(Test) {
  void TEST_FUNCTION(basic)
  {
    std::stringstream ss;
    // Redirect cerr to ss.
    do
    {
      redirect r(ss);
      Debug( check_configuration() );
      Debug( libcw_do.on() );
      Debug( dc::notice.on() );

      Dout(dc::notice, "Basic Test 0.");
      Dout(dc::notice, "Basic Test 1.");
    }
    while(0);
    // cerr no more redirected

    char const* exp[] = {
      "NOTICE  : Basic Test 0.",
      "NOTICE  : Basic Test 1.", 
      0
    };
    std::string line;
    for(int n = 0; getline(ss, line); ++n)
    {
      if(!exp[n])
      {
	TEST_ASSERT_M(exp[n], "less lines expected");
	break;
      }
      TEST_ASSERT_M(PatternCompareFunc(line, exp[n]), "'" << line << "' does not match '" << exp[n] << "'");
    }
  }
};

#endif // T_BASIC_H

