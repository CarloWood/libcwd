/***************************************************************************
    copyright            : (C) 2003 by Stefano Barbato
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

#ifndef _CUTEE_CUTEE_H_
#define _CUTEE_CUTEE_H_
#include <iostream>
#include <iomanip>
#include <cstdarg>

#define CUTEE_VERSION "0.4.0"

#define TEST_CLASS( name ) \
	dummy_##name {}; \
	static const char* class_name = #name; \
	struct name: public cutee::test_base
#define TEST_FUNCTION_EX( funcname, filename, lineno ) \
	run_##funcname() \
	{ \
		set_context(class_name, #funcname, filename, lineno); \
		testno(testno()+1); \
		print_func_info(); \
		m_func_exitcode = 0; \
		funcname(); \
		if(m_func_exitcode) s_func_failed++; else s_func_success++; \
	} \
	void funcname()


#define TEST_FUNCTION( funcname ) \
	TEST_FUNCTION_EX( funcname, __FILE__, __LINE__)

#define PRINT_VALUE( a ) \
	"\t[" << #a << ": " << a << "]" << std::endl

#define PRINT_ON_FAILURE_1( a ) \
	PRINT_VALUE( a ) 
#define PRINT_ON_FAILURE_2( a, b ) \
	PRINT_VALUE( a ) << PRINT_VALUE( b )
#define PRINT_ON_FAILURE_3( a, b, c ) \
	PRINT_VALUE( a ) << PRINT_VALUE( b ) << PRINT_VALUE( c )
#define PRINT_ON_FAILURE_4( a, b, c, d ) \
	PRINT_VALUE( a ) << PRINT_VALUE( b ) << \
	PRINT_VALUE( c ) << PRINT_VALUE( d )

#define TEST_ASSERT_EX( expr, file, line ) \
	if( !(expr) ) { test_failed(#expr, file, line); } else { test_success(#expr, file, line); } 

#define TEST_ASSERT_EX_M( expr, p, file, line ) \
	if( !(expr) ) { test_failed(#expr, file, line); std::cout << p << std::endl; } else { test_success(#expr, file, line); } 

#define TEST_ASSERT( expr ) TEST_ASSERT_EX( expr, __FILE__, __LINE__ ) 
#define TEST_ASSERT_EQUALS( a, b ) TEST_ASSERT_EX( (a == b), __FILE__,__LINE__ )
#define TEST_ASSERT_DIFFERS( a, b ) TEST_ASSERT_EX( (a != b),__FILE__,__LINE__ )

// print p on test fail
#define TEST_ASSERT_M( expr , p) TEST_ASSERT_EX_M( expr, p, __FILE__,__LINE__ ) 
#define TEST_ASSERT_EQUALS_M( a, b, p ) TEST_ASSERT_EX_M( (a == b), p,__FILE__,__LINE__ )
#define TEST_ASSERT_DIFFERS_M( a, b, p ) TEST_ASSERT_EX_M( (a != b), p,__FILE__,__LINE__)

// print a and b on error
#define TEST_ASSERT_P( expr ) TEST_ASSERT_EX_M( expr, PRINT_ON_FAILURE_1( expr ), __FILE__,__LINE__ ) 
#define TEST_ASSERT_EQUALS_P( a, b ) TEST_ASSERT_EX_M( (a == b), PRINT_ON_FAILURE_2( a, b ), __FILE__,__LINE__ )
#define TEST_ASSERT_DIFFERS_P( a, b ) TEST_ASSERT_EX_M( (a != b), PRINT_ON_FAILURE_2( a, b ), __FILE__,__LINE__)

#define TEST_ASSERT_PM( expr, m ) TEST_ASSERT_EX_M( expr, PRINT_ON_FAILURE_2( expr, m ), __FILE__,__LINE__ ) 
#define TEST_ASSERT_EQUALS_PM( a, b, m ) TEST_ASSERT_EX_M( (a == b), PRINT_ON_FAILURE_3( a, b, m ), __FILE__,__LINE__ )
#define TEST_ASSERT_DIFFERS_PM( a, b, m ) TEST_ASSERT_EX_M( (a != b), PRINT_ON_FAILURE_3( a, b, m ), __FILE__,__LINE__)

namespace cutee // c++ unit testing environment 
{

struct run_test
{
	virtual ~run_test() {}
	virtual int count() = 0;
	virtual void run() = 0;
};

struct add_test
{
	virtual ~add_test() {}
	static run_test* list[1000];
	static int list_idx;
};

struct test_base
{
	test_base()
	: m_testno(0)
	{
	}
	virtual ~test_base() 
	{
	}
	virtual void setUp() {}
	virtual void tearDown() {}
	// vars shared by all tests
	static int s_success, s_failed;
	static int s_func_success, s_func_failed;
protected:
	int m_func_exitcode;
	void test_success(const char* /*test*/, const char* /*file*/, int /*line*/)
	{
		s_success++;
	}
	void test_failed(const char* test, const char* file, int line)
	{
		s_failed++;
		m_func_exitcode++;
		std::cout << std::endl << " [" 
			<< file << ":" << line  << "] "
			<< cn() << "::" << fn() << "(): "
			<< test << " assertion failed" 
			<< std::endl;
	}
	void print_func_info() 
	{
		std::cout << "." << std::flush;
	}
	void print_header()
	{
		std::cout << cn() << ":" << std::endl;
	}
	void testno(int no)
	{
		m_testno = no;
	}
	int testno() const
	{
		return m_testno;
	}
	const char* cn() const
	{	return m_classname;	}
	const char* fn() const
	{	return m_fn;	}
	const char* file() const
	{	return m_file;	}
	int line() const
	{	return m_line;	}
	void set_context(
		const char* classname, 
		const char* fn, 
		const char* file, 
		int line)
	{
		m_classname = classname;	
		m_fn = fn;
		m_file = file;
		m_line = line;
	}
	const char *m_classname, *m_fn, *m_file;
	int m_line, m_testno;
};


struct results
{
	static void print()
	{
		using std::cout;
		using std::endl;
		using std::setw;
		for(int i =0; i < add_test::list_idx; i++)
			add_test::list[i]->run();
		int funcs = test_base::s_func_success + 
				test_base::s_func_failed;
		int checks = test_base::s_success + test_base::s_failed;
		cout << endl << endl;
		cout << "    ======================================" << endl;
		cout << "	      Tests Statistics           " << endl;
		cout << "    --------------------------------------" << endl;
		cout << "                Functions       Checks   " << endl;
		cout << "      Success   "<< setw(8) << 
			test_base::s_func_success << "      " << 
			setw(8) << test_base::s_success << endl;
		cout << "       Failed   "<< setw(8) << test_base::s_func_failed << 
			"      " << setw(8) << test_base::s_failed<< endl;
		cout << "    --------------------------------------" << endl;
		cout << "        Total   "<< setw(8) << funcs << "      " << 
			setw(8) << checks << endl;
		cout << "    ======================================" << endl;
	}
};


}


#endif
