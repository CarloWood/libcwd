#ifndef INSTANTIATE

#include "sys.h"
#include <iostream>
#include <libcw/debug.h>
#include <libcw/demangle.h>

#if __GXX_ABI_VERSION == 0
char const* test_cases [] = {
   "add_option__H1Z12libcw_app_ct_12libcw_app_ctPMX01FPX01PCc_vPCccT2T2_v",
   "_Q35libcw17_GLOBAL_.N.fn__Fvt23compiler_bug_workaround1Zt6vector2ZQ25libcwt13omanip_id_tct1ZQ35libcw5debug32memblk_types_manipulator_data_ctZt9allocator1ZQ25libcwt13omanip_id_tct1ZQ35libcw5debug32memblk_types_manipulator_data_ct.ids",
   "cwprint_using__H1ZQ35libcw10_internal_12GlobalObject_Q25libcw5debugRCX01PMX01CFPCX01R7ostream_v_Q35libcw5debugt17cwprint_using_tct1ZX01",
   "top__Ct14priority_queue3ZP27timer_event_request_base_ctZt5deque2ZP27timer_event_request_base_ctZt9allocator1ZP27timer_event_request_base_ctZ13timer_greater",
   "__eq__Ct15_Deque_iterator3ZP15memory_block_stZRCP15memory_block_stZPCP15memory_block_stRCt15_Deque_iterator3ZP15memory_block_stZRCP15memory_block_stZPCP15memory_block_st",
   "__mi__Ct17__normal_iterator2ZPC6optionZt6vector2Z6optionZt9allocator1Z6optionRCt17__normal_iterator2ZPC6optionZt6vector2Z6optionZt9allocator1Z6option",
   "_S_construct__H1ZPc_t12basic_string3ZcZt11char_traits1ZcZQ35libcw5debug27no_alloc_checking_allocatorX00X00RCQ35libcw5debug27no_alloc_checking_allocator_Pc",
   "f__H3Z1AZP1AZPC1A_X01X11X21PA4_PC1APO1C_PA4_PC1A_v",
   "foo__FiPiPPiPPPiPPPPiPPPPPiPPPPPPiPPPPPPPiPPPPPPPPiPPPPPPPPPiPPPPPPPPPPiPPPPPPPPPPPiPPPPPPPPPPPPiPPPPPPPPPPPPPiPPPPPPPPPPPPPPiPPPPPPPPPPPPPPPi",
   "B__H3Zt1D3ZP1AZRCP1AZPCP1AZt1D3ZP1AZRP1AZPP1AZP1A_X01X01X11PX21_t1D3ZX21ZRX21ZPX21",
   "SKIPPED",
   "_X11TransParseAddress"
};
#else
char const* test_cases [] = {
   "_ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_",
   "_ZGVN5libcw24_GLOBAL__N_cbll.cc0ZhUKa23compiler_bug_workaroundISt6vectorINS_13omanip_id_tctINS_5debug32memblk_types_manipulator_data_ctEEESaIS6_EEE3idsE",
   "_ZN5libcw5debug13cwprint_usingINS_10_internal_12GlobalObjectEEENS0_17cwprint_using_tctIT_EERKS5_MS7_FvRSoE",
   "_ZNKSt14priority_queueIP27timer_event_request_base_ctSt5dequeIS1_SaIS1_EE13timer_greaterE3topEv",
   "_ZNKSt15_Deque_iteratorIP15memory_block_stRKS1_PS2_EeqERKS5_",
   "_ZNKSt17__normal_iteratorIPK6optionSt6vectorIS0_SaIS0_EEEmiERKS6_",
   "_ZNSbIcSt11char_traitsIcEN5libcw5debug27no_alloc_checking_allocatorEE12_S_constructIPcEES6_T_S7_RKS3_",
   "_Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_M1DRA3_S3_",
   "_Z3fooiPiPS_PS0_PS1_PS2_PS3_PS4_PS5_PS6_PS7_PS8_PS9_PSA_PSB_PSC_",
   "_ZSt1BISt1DIP1ARKS2_PS3_ES0_IS2_RS2_PS2_ES2_ET0_T_SB_SA_PT1_",
   "_ZngILi42EEvN1AIXplT_Li2EEE1TE",
   "_X11TransParseAddress"
};
#endif

int main(void)
{
  std::string result;
  for (size_t i = 0; i < sizeof(test_cases)/sizeof(char const*); ++i)
  {
    libcw::debug::demangle_symbol(test_cases[i], result);
    std::cout << result << '\n';
    result.erase();
  }
  return 0;
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

// _ZN5libcw5debug13cwprint_usingINS_10_internal_12GlobalObjectEEENS0_17cwprint_using_tctIT_EERKS5_MS7_FvRSoE

namespace libcw {
  namespace _internal_ {
    class GlobalObject { public: void dummy(std::ostream&) const; };
  }
  namespace debug {
    template<typename T>
      class cwprint_using_tct { };
    template<typename T>
      cwprint_using_tct<T> cwprint_using(T const&, void (T::*)(std::ostream&) const);
  }
}

void h(void)
{
  // Instantiation.
  libcw::_internal_::GlobalObject dummy;
  (void)libcw::debug::cwprint_using(dummy, &libcw::_internal_::GlobalObject::dummy);
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

// _ZNSbIcSt11char_traitsIcEN5libcw5debug27no_alloc_checking_allocatorEE12_S_constructIPcEES6_T_S7_RKS3_

namespace libcw {
  namespace debug {
    class no_alloc_checking_allocator { };
  }
}
namespace std {
  template<typename T>
    class char_traits { };
  template<typename T, class _Traits, class _Alloc>
    class basic_string {
    public:
      template<typename T2>
	char* _S_construct(T2, T2, _Alloc const&) { return ""; }
    };
}

void l(void)
{
  // Instantiation.
  std::basic_string<char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator> dummy;
  char* cp;
  libcw::debug::no_alloc_checking_allocator alloc;
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

#if __GNUC__ > 2

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

#endif

#endif // INSTANTIATE
