#ifndef _T_BASIC_H__
#define _T_BASIC_H__
#include <iostream>
#include <streambuf>
#include <iomanip>
#include <sstream>
#include <string>
#include "cutee.h"
#include <libcwd/debug.h>

using namespace std;

// this is used to easily redirect cerr/cout to another ostream 
struct redirect
{
	redirect(ostream& os)
	{
		pSb = cerr.rdbuf(os.rdbuf());
	}
	~redirect()
	{
		cerr.rdbuf(pSb);
	}
private:
	streambuf* pSb;
};

bool PatternCompareFunc(const string& l, const string& r)
{
	//you can implement here a simple wildcard comparison func or use a 
	//more powerful regex library or whatever you need
	return l == r;
}

struct TEST_CLASS( libcwd )
{
	void TEST_FUNCTION( basic )
	{
		stringstream ss;
		// redirect cerr to ss
		do {
			redirect r(ss);
			Debug( check_configuration() );
			Debug( libcw_do.on() );
			Debug( dc::notice.on() );

			Dout(dc::notice, "Basic Test 0.");
			Dout(dc::notice, "Basic Test 1.");
		} while(0);
		// cerr no more redirected

		int n = 0;
		const char * exp[] =
		{
			"NOTICE  : Basic Test 0.",
			"NOTICE  : Basic Test 1.", 
			0
		};
		string line;
		for(int n = 0 ; getline(ss, line); ++n)
		{
			if(!exp[n])
			{
				TEST_ASSERT_M(exp[n], 
					"less lines expected");
				break;
			}
			TEST_ASSERT_M(PatternCompareFunc(line, exp[n]),
				"'" << line << "' does not match '" << 
				exp[n] << "'");
		}

	}
};


#endif

