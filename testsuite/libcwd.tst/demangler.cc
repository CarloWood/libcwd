#include <libcw/sys.h>
#include <iostream>
#include <libcw/debug.h>
#include <libcw/demangle.h>

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
   "_ZngILi42EEvN1AIXplT_Li2EEE1TE"
};

int main(void)
{
  std::string result;
  for (size_t i = 0; i < sizeof(test_cases)/sizeof(char const*); ++i)
  {
    libcw::debug::demangle_symbol(test_cases[i], result);
    std::cout << result << '\n';
    result.clear();
  }
  return 0;
}
