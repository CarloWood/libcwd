#ifndef INSTANTIATE

#include "sys.h"
#include <libcwd/debug.h>
#include <libcwd/demangle.h>
#include <iostream>

char const* test_cases [] = {
   "_ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_",
   "_ZGVN5libcw24_GLOBAL__N_cbll.cc0ZhUKa23compiler_bug_workaroundISt6vectorINS_13omanip_id_tctINS_5debug32memblk_types_manipulator_data_ctEEESaIS6_EEE3idsE",
   "_ZN6libcwd13cwprint_usingIN5libcw9_private_12GlobalObjectEEENS_17cwprint_using_tctIT_EERKS5_MS5_KFvRSt7ostreamE",
   "_ZNKSt14priority_queueIP27timer_event_request_base_ctSt5dequeIS1_SaIS1_EE13timer_greaterE3topEv",
   "_ZNKSt15_Deque_iteratorIP15memory_block_stRKS1_PS2_EeqERKS5_",
   "_ZNKSt17__normal_iteratorIPK6optionSt6vectorIS0_SaIS0_EEEmiERKS6_",
   "_ZNSbIcSt11char_traitsIcEN6libcwd27no_alloc_checking_allocatorEE12_S_constructIPcEES5_T_S6_RKS2_",
   "_Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_",
   "_Z3fooiPiPS_PS0_PS1_PS2_PS3_PS4_PS5_PS6_PS7_PS8_PS9_PSA_PSB_PSC_",
   "_ZSt1BISt1DIP1ARKS2_PS3_ES0_IS2_RS2_PS2_ES2_ET0_T_SB_SA_PT1_",
   "_ZngILi42EEvN1AIXplT_Li2EEE1TE",
   "_X11TransParseAddress",
   "_ZNSt13_Alloc_traitsISbIcSt18string_char_traitsIcEN6libcwd9_private_17allocator_adaptorIcSt24__default_alloc_templateILb0ELi327664EELb1EEEENS4_IS8_S6_Lb1EEEE15_S_instancelessE",
   "_GLOBAL__I__Z2fnv",
   "_Z1rM1GFivEMS_KFivES_M1HFivES1_4whatIKS_E5what2IS8_ES3_",
   "_Z1xINiEE",
   "_Z3fooIA6_KiEvA9_KT_rVPrS4_",
   "_Z1fILi5E1AEvN1CIXqugtT_Li0ELi1ELi2EEE1qE",
   "_Z1fILi5EEvN1AIXcvimlT_Li22EEE1qE",
   "_Z1fPFYPFiiEiE",
   "_Z1fI1XENT_1tES2_",
   "_Z1fILi5E1AEvN1CIXstN1T1tEEXszsrS2_1tEE1qE",
   "_Z1fILi1ELc120EEv1AIXplT_cviLd4028ae147ae147aeEEE",
   "_Z1fILi1ELc120EEv1AIXplT_cviLf3f800000EEE",
   "_Z9hairyfuncM1YKFPVPFrPA2_PM1XKFKPA3_ilEPcEiE",
   "_Z4dep9ILi3EEvP3fooIXgtT_Li2EEE"
};

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );
  Debug( libcw_do.on() );

  std::string result;
  for (size_t i = 0; i < sizeof(test_cases)/sizeof(char const*); ++i)
  {
    libcwd::demangle_symbol(test_cases[i], result);
#ifndef _REENTRANT
    std::cout << result << '\n';
#else
// We could do this:
//
//  pthread_mutex_lock(&cout_mutex);
//  std::cout << result << '\n';
//  pthread_mutex_unlock(&cout_mutex);
//
// but that would cause this output to
// follow 'continued' debug output and
// debug output that uses nonewline_cf
// on the same line:
//
// Thread1&2: NOTICE Loading a file... _X11TransParseAddress
// Thread1  : done.
//
// While using the below will result in the
// better readable:
//
// Thread1  : NOTICE Loading a file... <unfinised>
// Thread2  : _X11TransParseAddress
// Thread1  : NOTICE <continued> done.
//
//
// Or (although the use of nonewline_cf is discouraged in the multi-threaded case!)
//
// Thread1&2: NOTICE This line does not end on a newline_X11TransParseAddress
// Thread1  :  , but this does.
//
// will instead look like:
//
// Thread1  : NOTICE This line does not end on a newline<no newline>
// Thread2  : _X11TransParseAddress
// Thread1  :  , but this does.
//
    Dout(dc::always|noprefix_cf, result);
#endif
    result.erase();
  }
  EXIT(0);
}

#else // INSTANTIATE

namespace std {
  template<typename T>
    class allocator { };
  template<typename T, class _Alloc = allocator<T> >
    class vector {
    public:
      vector(void);
      void size(void) const { }
    };
  class ostream { };
  template<typename T, class _Queue, class _Comp>
    class priority_queue {
    public:
      void top(void) const { }
    };
  template<typename T, class _Alloc = allocator<T> >
    class deque {
    };
}

// _ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_

class libcw_app_ct {
public:
  void dummy(char const*) { }
  template<typename T>
    static void add_option(void (T::*)(char const*), char const*, char, char const*, char const*);
};

template<typename T>
  void libcw_app_ct::add_option(void (T::*)(char const*), char const*, char, char const*, char const*)
{
}

void fn(void)
{
  // Instantiation.
  libcw_app_ct::add_option(&libcw_app_ct::dummy, "", '\0', "", "");
}

// _ZGVN5libcw16_GLOBAL__N__Z1fv23compiler_bug_workaroundISt6vectorINS_13omanip_id_tctINS_5debug32memblk_types_manipulator_data_ctEEESaIS6_EEE3idsE

namespace libcw {
  namespace debug {
    class memblk_types_manipulator_data_ct { };
  }
  template<typename T>
    class omanip_id_tct { };
  namespace {
    template<typename T>
      class compiler_bug_workaround {
      public:
	static std::vector<int> ids;
      };
    template<typename T>
      std::vector<int> compiler_bug_workaround<T>::ids;
    typedef std::vector<libcw::omanip_id_tct<libcw::debug::memblk_types_manipulator_data_ct> > vector_t;
    compiler_bug_workaround<vector_t> dummy;
  }
}

void g(void)
{
  // Instantiation.
  libcw::dummy.ids.size();
}

// _ZN6libcwd13cwprint_usingIN5libcw9_private_12GlobalObjectEEENS_17cwprint_using_tctIT_EERKS5_MS5_KFvRSt7ostreamE

namespace libcw {
  namespace _private_ {
    class GlobalObject { public: void dummy(std::ostream&) const; };
  }
}
namespace libcwd {
  template<typename T>
    class cwprint_using_tct { };
  template<typename T>
    cwprint_using_tct<T> cwprint_using(T const&, void (T::*)(std::ostream&) const);
}

void h(void)
{
  // Instantiation.
  libcw::_private_::GlobalObject dummy;
  (void)libcwd::cwprint_using(dummy, &libcw::_private_::GlobalObject::dummy);
}

// _ZNKSt14priority_queueIP27timer_event_request_base_ctSt5dequeIS1_SaIS1_EE13timer_greaterE3topEv

class timer_event_request_base_ct { };
struct timer_greater { };

void i(void)
{
  // Instantiation.
  std::priority_queue<timer_event_request_base_ct*, std::deque<timer_event_request_base_ct*>, timer_greater> dummy;
  dummy.top();
}

// _ZNKSt15_Deque_iteratorIP15memory_block_stRKS1_PS2_EeqERKS5_

struct memory_block_st { };
namespace std {
  template<typename T, typename R = T const&, typename P = T const*>
    class _Deque_iterator {
    public:
      void operator==(_Deque_iterator const&) const { }
    };
}

void j(void)
{
  // Instantiation.
  std::_Deque_iterator<memory_block_st*> dummy;
  dummy.operator==(dummy);
}

// _ZNKSt17__normal_iteratorIPK6optionSt6vectorIS0_SaIS0_EEEmiERKS6_

struct option { };
namespace std {
  template<typename T1, typename T2>
    class __normal_iterator {
    public:
      void operator-(__normal_iterator const&) const { }
    };
}

void k(void)
{
  // Instantiation.
  std::__normal_iterator<option const*, std::vector<option> > dummy;
  dummy.operator-(dummy);
}

// _ZNSbIcSt11char_traitsIcEN6libcwd27no_alloc_checking_allocatorEE12_S_constructIPcEES5_T_S6_RKS2_

namespace libcwd {
  class no_alloc_checking_allocator { };
}
namespace std {
  template<typename T>
    class char_traits { };
  template<typename T, class _Traits, class _Alloc>
    class basic_string {
    public:
      template<typename T2>
	char* _S_construct(T2, T2, _Alloc const&) { return (char*)0; }
    };
}

void l(void)
{
  // Instantiation.
  std::basic_string<char, std::char_traits<char>, libcwd::no_alloc_checking_allocator> dummy;
  char* cp;
  libcwd::no_alloc_checking_allocator alloc;
  dummy._S_construct(cp, cp, alloc);
}

// Original test case: _Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_M1DRA3_S3_
// But because it is illegal to have a member-pointer to reference, this does:
// _Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_

class A { };
typedef A const* a4_t[4];
typedef a4_t* ap4_t;
class C {
public:
  ap4_t c;
};
template<typename T1, typename T2, typename T3>
  void f(T1, T2, T3, ap4_t, ap4_t (C::*)) { }

void m(void)
{
  // Instantiation.
  A a;
  a4_t a4;
  f(a, &a, static_cast<A const*>(&a), &a4, &C::c);
}

// _Z3fooiPiPS_PS0_PS1_PS2_PS3_PS4_PS5_PS6_PS7_PS8_PS9_PSA_PSB_PSC_

void foo(int, int*, int**, int***, int****, int*****, int******, int*******, int********, int*********, int**********, int***********, int************, int*************, int**************, int***************)
{
}

void n(void)
{
  int i0;
  int* i1;
  int** i2;
  int*** i3;
  int**** i4;
  int***** i5;
  int****** i6;
  int******* i7;
  int******** i8;
  int********* i9;
  int********** i10;
  int*********** i11;
  int************ i12;
  int************* i13;
  int************** i14;
  int*************** i15;
  foo(i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15);
}

// Original test case: _ZSt1BISt1DIP1ARKS2_PS3_ES0_IS2_RS2_PS2_ES2_ET0_T_SB_SA_PT1_
// Real mangled name : _ZSt1BISt1DIP1ARKS2_PS3_ES0_IS2_RS2_PS2_ES2_ES0_IT1_RSA_PSA_ET_SE_T0_SC_

namespace std {
  template<typename T1, typename T2, typename T3>
    class D { };
  D<A*, A*&, A**> d;
  template<typename T1, typename T2, typename T3>
    D<T3, T3&, T3*> B(T1, T1, T2, T3*) { return d; }
}

void o(void)
{
  std::D<A*, A* const&, A* const*> dummy1;
  std::D<A*, A*&, A**> dummy2;
  A* dummy3;
  std::B(dummy1, dummy1, dummy2, &dummy3);
}

// _ZngILi42EEvN1QIXplT_Li2EEE1TE

template<int i>
  struct Z { };
template<int i>
  struct Q {
    typedef Z<i> T;
    T t;
  };
template<int i>
  void operator-(typename Q<i + 2>::T) { }

void p(void)
{
  Q<42 + 2>::T z;
  operator-<42>(z);
}

namespace std {
  template<bool b, int i>
    class __default_alloc_template { };

  template<typename CHAR>
    class string_char_traits { };

  template<class BASIC_STRING, class ADAPTOR>
    struct _Alloc_traits {
      static char _S_instanceless;
    };
  template<class BASIC_STRING, class ADAPTOR>
    char _Alloc_traits<BASIC_STRING, ADAPTOR>::_S_instanceless;
}
namespace libcwd {
  namespace _private_ {
    template<typename CHAR, class ALLOCATOR, bool b>
      class allocator_adaptor { };
  }
}

char x;
void q(void)
{
  std::_Alloc_traits<
    std::basic_string<
      char,
      std::string_char_traits<char>,
      libcwd::_private_::allocator_adaptor<
	char,
        std::__default_alloc_template<false, 327664>,
	true
      >
    >,
    libcwd::_private_::allocator_adaptor<
      std::basic_string<
	char,
        std::string_char_traits<char>,
	libcwd::_private_::allocator_adaptor<
	  char,
	  std::__default_alloc_template<false, 327664>,
	  true
	>
      >,
      std::__default_alloc_template<false, 327664>,
      true
    >
  > dummy1;
  x = dummy1._S_instanceless;
}

// _Z1rM1GKFivE

struct G {
  int m(void) const { return 0; }
  int n(void) { return 0; }
};

struct H {
  int m(void) const { return 0; }
  int n(void) { return 0; }
};

template<typename T>
  class what
  {
  };

template<typename T>
  class what2
  {
  };

void r(int (G::*)(void),
       int (G::*)(void) const,
       G, /* S_ */
       int (H::*)(void), /* M1HS0_ */
       int (G::*)(void), /* S1_ */
       what<G const>, /* what<S2_> */
       what2<G const>, /* what2<S8_> */
       int (G::*)(void) const /* S3_ */)
{
  G g;
  what<G const> y;
  what2<G const> y2;
  r(&G::n, &G::m, g, &H::n, &G::n, y, y2, &G::m);
}

#endif // INSTANTIATE
