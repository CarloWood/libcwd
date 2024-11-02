/***************************************************************************
    copyright            : (C) by 2002-2003 Stefano Barbato
    email                : stefano@codesink.org

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <iostream>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <string>
#include <list>
#include <ctype.h>
#include <getopt.h>
#include "cutee.h"

#define DEFAULT_RUNNER_EXT 	"cutee.cc"
#define RUNTEST_NAME		"runtest"
#define CPP_EXT			" cc cxx cpp c++ CC CXX CPP C++ "

using namespace std;

// c++ unit testing easy environment
const char* g_usage = 
"cutee, C++ Unit Testing Easy Environment, Ver. " CUTEE_VERSION  "\n\
Copyright (c) 2003 by Stefano Barbato - All rights reserved.\n\
Usage: cutee [options]... testfile(s)\n\
 -m, --main             generates runtest source code\n\
 -o, --output=filename  specify the output filename\n\
 -p, --makefile         generates autocutee.mk to include in your Makefile\n\
 -k, --automakefile     generates autocutee.mk to include in your Makefile.am\n\n";

typedef list<string> StringList;

#define die(msg) do { do_die_if(1, msg, __LINE__); } while(0);
#define die_if(a, msg) do { do_die_if(a, msg, __LINE__); } while(0);
void do_die_if(int b, const string& msg, int line)
{
	if(!b)
		return;
	cerr << "(" << line << ") " << msg << endl;
	exit(1);
}
#define _( code ) of << code << endl

enum { 	
	MODE_RUNTEST, 
	MODE_MAIN, 
	MODE_AUTOMAKEFILE, 
	MODE_MAKEFILE  
};

struct CmdLineOpts
{
	string ifile, ofile, ext;
	string class_name;
	StringList ifList;
	int mode;

	CmdLineOpts()
	: ext(DEFAULT_RUNNER_EXT),mode(MODE_RUNTEST)
	{
	}
	void parse(int argc, char **argv)
	{
		enum { OPT_NO_ARG, OPT_ARG_REQUIRED, OPT_ARG_OPTIONAL };
		struct option opts[] =
			{
				{ "main", OPT_NO_ARG, NULL, 'm'},
				{ "automakefile", OPT_NO_ARG, NULL, 'k'},
				{ "makefile", OPT_NO_ARG, NULL, 'p'},
				{ "output", OPT_ARG_REQUIRED, NULL, 'o'},
				{ 0, 0, 0, 0 }
			};
		int ret;
		mode = MODE_RUNTEST; // default
		while((ret=getopt_long(argc, argv, "mo:pk",opts,NULL)) != -1)
		{
			switch(ret)
			{
			case 'o':
				ofile = optarg;
				break;
			case 'm':
				mode = MODE_MAIN;
				break;
			case 'k':
				mode = MODE_AUTOMAKEFILE;
				break;
			case 'p':
				mode = MODE_MAKEFILE;
				break;
			default:
				die(g_usage);
			}
		}
		switch(mode)
		{
		case MODE_RUNTEST:
			die_if( (argc - optind) != 1 || ofile.empty(), g_usage);
			for(; optind < argc; optind++)
				ifile = argv[optind];
			break;
		case MODE_MAIN:
			die_if( (argc - optind) != 0 || ofile.empty(), g_usage);
			break;
		case MODE_MAKEFILE:
			die_if( (argc - optind) < 1 || ofile.empty(), g_usage);
			for(; optind < argc; optind++)
				ifList.push_back(argv[optind]);
			break;
		};
	}
};


class String2WordList
{
	list<string> m_list;
	string m_s;
public:
	typedef list<string> WordList;
	typedef list<string>::iterator iterator;
	typedef list<string>::const_iterator const_iterator;

	String2WordList(const string& s)
	: m_s(s)
	{
		if(m_s.empty())
			return;
		split();
	}
	inline bool isWordChar(char c) const
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
			(c >= '0' && c <= '9') || (c == '_');
	}
	void split()
	{
		string word;
		string::const_iterator beg = m_s.begin(), end = m_s.end();
		for( ; beg != end; ++beg)
		{
			if(isWordChar(*beg))	
				word += *beg;
			else {
				if(word.empty())
					continue;
				m_list.push_back(word);
				word.erase(word.begin(), word.end());
			}
		}
		if(!word.empty())
			m_list.push_back(word);
	}
	iterator begin() 		{	return m_list.begin();	}
	iterator end()			{	return m_list.end();	}
	const_iterator begin() const	{	return m_list.begin();	}
	const_iterator end() const	{	return m_list.end();	}
};

struct GenMain
{
	GenMain(const string& ofqn)
	: m_ofqn(ofqn)
	{
	}
	void writeMain()
	{
		ofstream of(m_ofqn.c_str());
		die_if(!of.is_open(), 
			string("Unable to open output file: ") + m_ofqn);

		_( "#include <iostream>" );
		_( "#include <iomanip>" );
		_( "#include \"cutee.h\"" );
		_( "using namespace std;" );
		_( "using namespace cutee;" );
		_( "" );
		_( "// static vars initialization" );
		_( "run_test* add_test::list[1000];" );
		_( "int add_test::list_idx = 0;" );
		_( "int test_base::s_success = 0;" );
		_( "int test_base::s_failed = 0;" );
		_( "int test_base::s_func_success = 0;" );
		_( "int test_base::s_func_failed = 0;" );
		_( "" );
		_( "int main() " );
		_( "{" );
		_( "\tresults::print();" );
		_( "}" );
	}
private:
	string m_ofqn;
};

struct GenRunTest
{
	GenRunTest(const string& ifqn, const string& ofqn)
	: m_ifqn(ifqn), m_ofqn(ofqn)
	{
	}
	void writeRunTestClass()
	{
		ofstream of(m_ofqn.c_str());
		die_if(!of.is_open(), 
			string("Unable to open output file: ") + m_ofqn);
		string cn = m_className;
		_( "#include \"cutee.h\"" );
		_( "#include \"" << m_ifqn << "\"" );
		_( "struct run_" << cn << ": public cutee::run_test");
		_( "{" );
		_( "  void run()" );
		_( "  {" );
		of << "    ";
		if(m_nsList.empty())
			of << cn;
		else {		
			StringList::const_iterator 
					beg = m_nsList.begin(),
					end = m_nsList.end();
			for(; beg != end; ++beg)
				of << *beg << "::";
			of << cn;
		}
		of << " t;" << endl;
		_( "    t.setUp();" );
		StringList::const_iterator 
					beg = m_fnList.begin(),
					end = m_fnList.end();
		for(; beg != end; ++beg)
			_( "    t.run_" << *beg << "();" );
		_( "    t.tearDown();" );
		_( "  }" );
		_( "  int count() { return " << m_fnList.size() << "; }" );
		_( "};" );
		_( "struct " << cn << "_add_to_list: public cutee::add_test");
		_( "{" );
		_( "  " << cn << "_add_to_list()" );
		_( "  {" );
		_( "  list[list_idx++] = new run_" << cn << ";" );
		_( "  }" );
		_( "} " << cn << "_addit;" );
	}
	int parseInputFile()
	{
		enum { 
			START, GET_NAMESPACE_NAME, GET_CLASS_NAME, 
			WAITING_FUNCTION, GET_FUNCTION_NAME
		};
		ifstream inf(m_ifqn.c_str());
		die_if(!inf.is_open(), 
			string("Unable to open input file: ") + m_ifqn);

		int state = START;
		string line;
		while(getline(inf, line))
		{
			string w, uw, prev;
			String2WordList s2wl(line);
			String2WordList::iterator 
					beg = s2wl.begin(), end = s2wl.end();
			while( beg != end )
			{
				prev = w;
				w = *beg;
				switch(state)
				{
				case START:
					if(w == "namespace" && prev != "using")
						state = GET_NAMESPACE_NAME;
					else if(w == "TEST_CLASS")
						state = GET_CLASS_NAME;
					break;
				case GET_NAMESPACE_NAME:
					m_nsList.push_back(w);
					state = START;
					break;
				case GET_CLASS_NAME:
					m_className = w;
					state = WAITING_FUNCTION;
					break;
				case WAITING_FUNCTION:
					if(w == "TEST_FUNCTION")
						state = GET_FUNCTION_NAME;
					break;
				case GET_FUNCTION_NAME:
					m_fnList.push_back(w);
					state = WAITING_FUNCTION;
					break;
				default:
					die("Unknown state");
				}
				++beg;
			}
		}
		return !m_className.empty();
	}
private:
	string m_ifqn, m_ofqn, m_className;
	StringList m_nsList, m_fnList;
};

struct GenMakefile
{
	GenMakefile(const StringList& ifList, const string& ofqn, const string& ext)
	: m_ofqn(ofqn), m_ext(ext), m_ifList(ifList)  
	{
	}
	void writeMakefile()
	{
		string cc_exts = CPP_EXT;
		ofstream of(m_ofqn.c_str());
		die_if(!of.is_open(), 
			string("Unable to open output file: ") + m_ofqn);

		_( "# cutee autogen: begin" );
		_( "CUTEE=./cutee" );

		of << "object_files=";
		StringList::const_iterator 
				beg = m_ifList.begin(),
				end = m_ifList.end();
		for(; beg != end; ++beg)
		{
			GenRunTest grt(*beg, "");
			if(grt.parseInputFile())
				of << stripExt(*beg) << "." << stripExt(m_ext) 
					<< ".o" << " ";
			else if(cc_exts.find(" " + getExt(*beg) + " ") != string::npos)
				of << stripExt(*beg) << "." << "o" << " ";
		}
		of << endl;

		beg = m_ifList.begin(),	end = m_ifList.end();
		for(; beg != end; ++beg)
		{
			_( "" );
			GenRunTest grt(*beg, "");
			if(grt.parseInputFile())
			{
				string dst = stripExt(*beg) + "." + m_ext;
				of << dst << ": " << *beg << endl;
				of << "\t$(CUTEE) -o " << dst << " " << *beg 
				<< endl;
			}

		}
		of << endl;

		_( "" );
		_( RUNTEST_NAME ".cc: cutee" );
		_( "\t$(CUTEE) --main -o " RUNTEST_NAME ".cc" );
		_( "" );
		_( RUNTEST_NAME ": autocutee.mk " RUNTEST_NAME ".o $(object_files)");
		_( "\t$(CXX) $(CXXFLAGS) -o " RUNTEST_NAME " " RUNTEST_NAME ".o $(object_files)");
		_( "\t./" RUNTEST_NAME );
		_( "" );
		_( "# cutee autogen: end ");
	}
private:
	string getExt(const string& fqn)
	{
		string::size_type idx =	fqn.find_last_of('.');
		if(idx != string::npos)
			return string(fqn, ++idx);
		else
			return string();
	}
	string stripExt(const string& fqn)
	{
		string::size_type idx =	fqn.find_last_of('.');
		if(idx != string::npos)
			return string(fqn, 0, idx);
		else
			return fqn;
	}
	string m_ofqn, m_ext;
	StringList m_ifList;
};

struct GenAutomakefile
{
	GenAutomakefile(const StringList& ifList, const string& ofqn, const string& ext)
	: m_ofqn(ofqn), m_ext(ext), m_ifList(ifList)
	{
	}
private:
	string stripPath(const string& fqn)
	{
		string::size_type idx =	fqn.find_last_of('/');
		if(idx != string::npos)
			return string(fqn, ++idx);
		else
			return fqn;
	}
	string stripExt(const string& fqn)
	{
		string::size_type idx =	fqn.find_last_of('.');
		if(idx != string::npos)
			return string(fqn, 0, idx);
		else
			return fqn;
	}
	string m_ofqn, m_ext;
	StringList m_ifList;
};
//
// main
//
int main(int argc, char** argv)
{
	CmdLineOpts clo;
	clo.parse(argc,argv);
	GenRunTest* pGrt;
	GenMain* pGm;
	GenMakefile* pGmk;


	switch(clo.mode)
	{
	case MODE_RUNTEST:
		// i'm using new/delete to avoid a warning (jump to case 
		// label crosses initialization of ...) that throw an
		// internal compiler error using gcc 3.2(linux)
		pGrt = new GenRunTest(clo.ifile, clo.ofile);
		if(pGrt->parseInputFile())
			pGrt->writeRunTestClass();
		delete pGrt;
		break;
	case MODE_MAIN:
		pGm = new GenMain(clo.ofile);
		pGm->writeMain();
		delete pGm;
		break;
	case MODE_MAKEFILE:
		pGmk = new GenMakefile(clo.ifList, clo.ofile, clo.ext);
		pGmk->writeMakefile();
		delete pGmk;
		break;
	default:
		die("UNKNOWN MODE");
		break;
	}
	return 0;
}


