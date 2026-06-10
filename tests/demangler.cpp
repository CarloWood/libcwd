// Verify libcwd::demangle_symbol against the legacy demangler regression cases.
//
// The test feeds a list of curated mangled names through libcwd::demangle_symbol
// and exact-matches the current maintained demangled spelling for each case.
// It has no side effects beyond diagnostics on mismatch.

#include "cwd_sys.h"
#include "test_support.h"
#include <libcwd/demangle.h>

#include <cstdlib>
#include <sstream>
#include <string>

namespace {

char const* test_cases[] = {
    "_ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_",
    "_ZGVN5libcw24_GLOBAL__N_cbll.cc0ZhUKa23compiler_bug_workaroundISt6vectorINS_13omanip_id_tctINS_5debug32memblk_"
    "types_manipulator_data_ctEEESaIS6_EEE3idsE",
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
    "_ZNSt13_Alloc_traitsISbIcSt18string_char_traitsIcEN6libcwd9_private_17allocator_adaptorIcSt24__default_alloc_"
    "templateILb0ELi327664EELb1EEEENS4_IS8_S6_Lb1EEEE15_S_instancelessE",
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
    "_Z4dep9ILi3EEvP3fooIXgtT_Li2EEE"};

// Demangle every test case into captured, one result per line.
//
// Unknown or intentionally unsupported names are expected to be copied through by
// demangle_symbol, so all cases are compared uniformly as text.
void write_demangled_output(std::ostream& captured)
{
  std::string result;
  for (char const* test_case : test_cases)
  {
    libcwd::demangle_symbol(test_case, result);
    captured << result << '\n';
    result.erase();
  }
}

} // namespace

int main()
{
  Debug(main_reached());
  Debug(libcw_do.on());

  std::stringstream captured;
  write_demangled_output(captured);

  char const* expected[] = {
      "void libcw_app_ct::add_option<libcw_app_ct>(void (libcw_app_ct::*)(char const*), char const*, char, char "
      "const*, char const*)",
      "guard variable for libcw::(anonymous "
      "namespace)::compiler_bug_workaround<std::vector<libcw::omanip_id_tct<libcw::debug::memblk_types_manipulator_"
      "data_ct>, std::allocator<libcw::omanip_id_tct<libcw::debug::memblk_types_manipulator_data_ct> > > >::ids",
      "libcwd::cwprint_using_tct<libcw::_private_::GlobalObject> "
      "libcwd::cwprint_using<libcw::_private_::GlobalObject>(libcw::_private_::GlobalObject const&, void "
      "(libcw::_private_::GlobalObject::*)(std::ostream&) const)",
      "std::priority_queue<timer_event_request_base_ct*, std::deque<timer_event_request_base_ct*, "
      "std::allocator<timer_event_request_base_ct*> >, timer_greater>::top() const",
      "std::_Deque_iterator<memory_block_st*, memory_block_st* const&, memory_block_st* "
      "const*>::operator==(std::_Deque_iterator<memory_block_st*, memory_block_st* const&, memory_block_st* const*> "
      "const&) const",
      "std::__normal_iterator<option const*, std::vector<option, std::allocator<option> > "
      ">::operator-(std::__normal_iterator<option const*, std::vector<option, std::allocator<option> > > const&) const",
      "char* std::basic_string<char, std::char_traits<char>, "
      "libcwd::no_alloc_checking_allocator>::_S_construct<char*>(char*, char*, libcwd::no_alloc_checking_allocator "
      "const&)",
      "void f<A, A*, A const*>(A, A*, A const*, A const* (*) [4], A const* (* C::*) [4])",
      "foo(int, int*, int**, int***, int****, int*****, int******, int*******, int********, int*********, "
      "int**********, int***********, int************, int*************, int**************, int***************)",
      "std::D<A*, A*&, A**> std::B<std::D<A*, A* const&, A* const*>, std::D<A*, A*&, A**>, A*>(std::D<A*, A* const&, "
      "A* const*>, std::D<A*, A* const&, A* const*>, std::D<A*, A*&, A**>, A**)",
      "void operator-<42>(A<(42) + (2)>::T)",
      "_X11TransParseAddress",
      "std::_Alloc_traits<std::basic_string<char, std::string_char_traits<char>, "
      "libcwd::_private_::allocator_adaptor<char, std::__default_alloc_template<false, 327664>, true> >, "
      "libcwd::_private_::allocator_adaptor<std::basic_string<char, std::string_char_traits<char>, "
      "libcwd::_private_::allocator_adaptor<char, std::__default_alloc_template<false, 327664>, true> >, "
      "std::__default_alloc_template<false, 327664>, true> >::_S_instanceless",
      "global constructors keyed to _Z2fnv",
      "r(int (G::*)(), int (G::*)() const, G, int (H::*)(), int (G::*)(), what<G const>, what2<G const>, int (G::*)() "
      "const)",
      "_Z1xINiEE",
      "void foo<int const [6]>(int const [9][6], int const restrict (* volatile restrict) [9][6])",
      "void f<5, A>(C<(((5) > (0))) ? (1) : (2)>::q)",
      "void f<5>(A<(int)((5) * (22))>::q)",
      "f(int (*(*) [extern \"C\"] (int))(int))",
      "X::t f<X>(X::t)",
      "void f<5, A>(C<sizeof (T::t), sizeof (T::t)>::q)",
      "void f<1, (char)120>(A<(1) + ((int)((double)1.234e+1))>)",
      "void f<1, (char)120>(A<(1) + ((int)((float)1))>)",
      "hairyfunc(int (* const (X::** (* restrict (* volatile* (Y::*)(int) const)(char*)) [2])(long) const) [3])",
      "void dep9<3>(foo<((3) > (2))>*)",
      nullptr};

  return libcwd_ctest::matches_expected_output(captured, expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
