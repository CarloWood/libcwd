// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <list>
#include <iomanip>
#include <sstream>
#include <libcw/debug.h>
#include <libcw/perf.h>
#include <papi_internal.h>
#include <papiStdEventDefs.h>

using std::cout;
using std::endl;
using std::ends;
using std::setw;
using std::setprecision;
using std::ostream;
using std::ofstream;

#undef TEST_STUDENT_T

static float student_t[5][34] = {
 { 3.078, 1.886, 1.638, 1.533,
   1.476, 1.440, 1.415, 1.397, 1.383,
   1.372, 1.363, 1.356, 1.350, 1.345,
   1.341, 1.337, 1.333, 1.330, 1.328,
   1.325, 1.323, 1.321, 1.319, 1.318,
   1.316, 1.315, 1.314, 1.313, 1.311,
   1.310, 1.303, 1.296, 1.289, 1.282 },
 { 6.314, 2.920, 2.353, 2.132,
   2.015, 1.943, 1.895, 1.860, 1.833,
   1.812, 1.796, 1.782, 1.771, 1.761,
   1.753, 1.746, 1.740, 1.734, 1.729,
   1.725, 1.721, 1.717, 1.714, 1.711,
   1.708, 1.706, 1.703, 1.701, 1.699,
   1.697, 1.684, 1.671, 1.658, 1.645 },
 { 12.706, 4.303, 3.182, 2.776,
   2.571, 2.447, 2.365, 2.306, 2.262,
   2.228, 2.201, 2.179, 2.160, 2.145,
   2.131, 2.120, 2.110, 2.101, 2.093,
   2.086, 2.080, 2.074, 2.069, 2.064,
   2.060, 2.056, 2.052, 2.048, 2.045,
   2.042, 2.021, 2.000, 1.980, 1.960 },
 { 31.821, 6.965, 4.541, 3.747,
   3.365, 3.143, 2.998, 2.896, 2.821,
   2.764, 2.718, 2.681, 2.650, 2.624,
   2.602, 2.583, 2.567, 2.552, 2.539,
   2.528, 2.518, 2.508, 2.500, 2.492,
   2.485, 2.479, 2.473, 2.467, 2.462,
   2.457, 2.423, 2.390, 2.358, 2.326 },
 { 63.657, 9.925, 5.841, 4.604,
   4.032, 3.707, 3.499, 3.355, 3.250,
   3.169, 3.106, 3.055, 3.012, 2.977,
   2.947, 2.921, 2.898, 2.878, 2.861,
   2.845, 2.831, 2.819, 2.807, 2.797,
   2.787, 2.779, 2.771, 2.763, 2.756,
   2.750, 2.704, 2.660, 2.617, 2.576 } };

static float student_t_certainty[5] = { 0.1, 0.05, 0.025, 0.01, 0.005 };	// Right cross over chance.

static float t(int certainty_index, int freedoms)
{
  if (freedoms <= 30)
    return student_t[certainty_index][freedoms - 1];
  double a, b, y1, y2, y3;
  long x1, x2;
  long x3 = 0;
  int i;
  if (freedoms <= 60)
  {
    i = 29;
    x1 = 30;
    x2 = 40;
    x3 = 60;
  }
  else if (freedoms <= 120)
  {
    i = 30;
    x1 = 40;
    x2 = 60;
    x3 = 120;
  }
  else
  {
    i = 31;
    x1 = 60;
    x2 = 120;
    /* x3 = infinity */
  }
  y1 = student_t[certainty_index][i];
  y2 = student_t[certainty_index][i + 1];
  y3 = student_t[certainty_index][i + 2];
  if (freedoms <= 120)
  {
    double c, d;
    d =   (x1 * x1 * (x3 - x2) + x2 * x2 * (x1 - x3) + x3 * x3 * (x2 - x1));
    a = - (x1      * (y3 - y2) + x2      * (y1 - y3) + x3      * (y2 - y1)) / d;
    b =   (x1 * x1 * (y3 - y2) + x2 * x2 * (y1 - y3) + x3 * x3 * (y2 - y1)) / d;
    c = y2 - a * x2 * x2 - b * x2;
    return (a * freedoms * freedoms + b * freedoms + c);
  }
  double ln1, ln2;
  ln1 = log(y2 - y3);
  ln2 = log(y1 - y3);
  a = - (     ln1 -      ln2) / (x1 - x2);
  b =   (x1 * ln1 - x2 * ln2) / (x1 - x2);
  return (y3 + exp(a * freedoms + b));
}

#ifdef TEST_STUDENT_T
#include <iostream>

int main(int argc, char** argv)
{
  int index = 4;
  if (argc < 2)
  {
    cerr << "Usage: " << argv[0] << " [0..4] degree_of_freedom\n";
    return -1;
  }
  if (argc == 3)
  {
    index = atoi(argv[1]);
    ++argv;
  }
  cout << "Crossover chance: " << student_t_certainty[index] << endl;
  long freedom = atol(argv[1]);
  cout << "Degrees of freedom: " << freedom << endl;
  cout << "t_" << student_t_certainty[index] << "(" << freedom << ") = " << t(index, freedom) << endl;
  return 0;
}
#endif

union data_ut {
  unsigned long long i_data;
  data_ut(unsigned long long data) : i_data(data) { }
};

unsigned int const PAPI_CUSTOM1 = PAPI_MAX_PRESET_EVENTS + 1;
unsigned int const PAPI_max_event = PAPI_CUSTOM1 + 1;

char const* PAPI_event_name(unsigned int event)
{
  event &= ~PRESET_MASK;
  if (event < PAPI_MAX_PRESET_EVENTS)
  {
    PAPI_preset_info_t const* info = PAPI_query_all_events_verbose();
    for(PAPI_preset_info_t const* p = info; p < &info[64]; ++p)
      if (p->avail && (p->event_code & ~PRESET_MASK) == event)
        return p->event_name + 5;
  }
  else
    switch(event)
    {
      case PAPI_CUSTOM1:
	return "cycles - elapsed";
    }
  return "Unknown event";
}

static std::vector<data_ut> count_array[PAPI_max_event];

unsigned int const PAPI_NONE = PAPI_max_event;

struct event_st {
  unsigned int event;
  int load;
};

// Events available on a Pentium Pro.
// Events are measured in pairs.
static struct event_st available_events[] = {
//  Name          Load       Code            Avail   Deriv   Description (Note)
#if 0
  { PAPI_L1_DCM,  1 },    // 0x80000000      Yes     No      Level 1 data cache misses (0x45)
  { PAPI_L1_ICM,  1 },    // 0x80000001      Yes     No      Level 1 instruction cache misses (0x81)
  { PAPI_L2_TCM,  1 },    // 0x80000007      Yes     No      Level 2 cache misses (0x24)
  { PAPI_TLB_IM,  1 },    // 0x80000015      Yes     No      Instruction translation lookaside buffer misses (0x85)
  { PAPI_MEM_SCY, 1 },    // 0x80000022      Yes     No      Cycles Stalled Waiting for Memory Access (0xa2)
  { PAPI_BR_TKN,  1 },    // 0x8000002c      Yes     No      Conditional Branch Instructions Taken (0xc9)
  { PAPI_BR_MSP,  1 },    // 0x8000002e      Yes     No      Conditional Branch Instructions Mispredicted (0xc5)
  { PAPI_TOT_IIS, 1 },    // 0x80000031      Yes     No      Instructions issued (0xd0)
  { PAPI_TOT_INS, 1 },    // 0x80000032      Yes     No      Instructions completed (0xc0)
  { PAPI_FP_INS,  1 },    // 0x80000034      Yes     No      Floating Point Instructions (0xc1)
  { PAPI_BR_INS,  1 },    // 0x80000037      Yes     No      Branch Instructions (0xc4)
  { PAPI_VEC_INS, 1 },    // 0x80000038      Yes     No      Vector Instructions (0xb0)
#endif
  { PAPI_TOT_CYC, 0 },    // 0x8000003c      Yes     No      Total cycles (0x79)
#if 0
  { PAPI_LST_INS, 1 },    // 0x8000003e      Yes     No      Load/store instructions completed (0x43)
  { PAPI_L1_TCM,  2 },    // 0x80000006      Yes     Yes     Level 1 cache misses (0x45,0x81)
  { PAPI_FLOPS,   2 },    // 0x80000039      Yes     Yes     Floating point instructions per second (0x79,0xc1)
  { PAPI_IPS,     2 },    // 0x8000003d      Yes     Yes     Instructions per second (0x79,0xc0)
#endif
};

static int const number_of_available_events = sizeof(available_events)/sizeof(struct event_st);

long long PERF_result_list[4];
int PERF_eventset_count = -1;
int PERF_max_eventset_count = 0;
int PERF_eventsets[64];

struct event_status {
  unsigned int event;
  int count;
  event_status(unsigned int ev) : event(ev), count(0) { }
  bool operator<(event_status& es) const { return count < es.count; }
  friend ostream& operator<<(ostream& os, event_status const& ev) { os << PAPI_event_name(ev.event) << "; " << ev.count; return os; }
};

std::string header;
static bool _G_plot;

void PERFstats_init(char const* name, bool plot = true)
{
  header = name;
  _G_plot = plot;
  if (plot)
  {
    std::ostringstream date;
    struct tm inittime;
    time_t now = time(NULL);
    inittime = *localtime(&now);
    date.width(2);
    date.fill('0');
    date << (inittime.tm_mon + 1);
    date.width(2);
    date << inittime.tm_mday;
    date.width(2);
    date << inittime.tm_hour;
    date.width(2);
    date << inittime.tm_min;
    date.width(2);
    date << inittime.tm_sec << ends;
    mkdir(date.str().c_str(), 0755);
    chdir(date.str().c_str());
  }
  PAPI_init();
  for (int i = 0; i < 64; ++i)
    PERF_eventsets[i] = PAPI_NULL;
  std::list<event_status> event_list;
  for (int c = 0; c < number_of_available_events; ++c)
    event_list.push_back(event_status(available_events[c].event));
  for(;;)
  {
    int count = 0;
    for (std::list<event_status>::iterator i(event_list.begin()); i != event_list.end(); ++i)
    {
      int retval = PAPI_add_event(&PERF_eventsets[PERF_max_eventset_count], (*i).event);
      if (retval < PAPI_OK)
	continue;
      count++;
      (*i).count++;
    }
    if (!count)
      break;
    ++PERF_max_eventset_count;
    event_list.sort();
    if (event_list.front().count > 0)
      break;
  }
}

void PERFstats_store(void)
{
  int events[3];
  int count = 3;
  PAPI_list_events(PERF_eventsets[PERF_eventset_count], events, &count);
  for (int i = 0; i < count; ++i)
    count_array[(events[i] & 0xff)].push_back(data_ut(PERF_result_list[i]));
}

static int normal_distribution(int event, int certainty_index, double cut_off_high, double cut_off_low, double& x_avg, double& s_n1)
{
  size_t n = count_array[event].size();
  double x_sum = 0;
  for (std::vector<data_ut>::iterator iter(count_array[event].begin()); iter != count_array[event].end(); ++iter)
  {
    double count = (*iter).i_data;
    if (count > cut_off_high || count < cut_off_low)
      --n;
    else
      x_sum += count;
  }
  x_avg = x_sum/n;
  double vtn1 = 0; // variation times n - 1
  for (std::vector<data_ut>::iterator iter(count_array[event].begin()); iter != count_array[event].end(); ++iter)
  {
    double count = (*iter).i_data;
    if (count > cut_off_high || count < cut_off_low)
      continue;
    vtn1 += (count - x_avg) * (count - x_avg);
  }
  s_n1 = sqrt(vtn1 / (n - 1));
  return n;
}

struct stats_st {
  int n;
  double x_avg;
  double s_n1;
  double cut_off_low;
  double cut_off_high;
  double xright;
  double xleft;
};

static stats_st determine_stats(int certainty_index, unsigned int event)
{
  stats_st stats;
  stats.x_avg = 0;
  stats.s_n1 = 1e30;
  double prev_s_n1;
  do {
    prev_s_n1 = stats.s_n1;
    stats.cut_off_high = stats.x_avg + 3 * stats.s_n1;
    stats.cut_off_low = stats.x_avg - 3 * stats.s_n1;
    stats.n = normal_distribution(event, certainty_index, stats.cut_off_high, stats.cut_off_low, stats.x_avg, stats.s_n1);
#ifdef DEBUGPERF
    cout << "x_avg = " << stats.x_avg << "; s_n1 = " << stats.s_n1 << '\n';
#endif
  }
  while(stats.n > 0 && stats.s_n1 != prev_s_n1);

  stats.xleft = 1e30;
  stats.xright = 0;
  for (std::vector<data_ut>::iterator iter(count_array[event].begin()); iter != count_array[event].end(); ++iter)
  {
    double count = (*iter).i_data;
    if (stats.xleft > stats.xright || stats.xleft > count)
      stats.xleft = count;
    if (stats.xright < stats.xleft || stats.xright < count)
      stats.xright = count;
  }
  if (stats.xright < stats.cut_off_high)
    stats.cut_off_high = stats.xright;
  if (stats.xleft > stats.cut_off_low)
    stats.cut_off_low = stats.xleft;
#ifdef DEBUGPERF
  cout << setw(18) << PAPI_event_name(event) << ' ' << setw(8) << stats.cut_off_low << ", " << setw(8) << stats.cut_off_high << " (" << stats.xleft << ", " << stats.xright << ')' << endl;
#endif
  stats.xright = stats.cut_off_high;
  stats.xleft = stats.cut_off_low;

  return stats;
}

#include <string>
#include <fstream>

void PERFstats_plot(int certainty_index)
{
  cout << "Intervals of " << 100 - 200 * student_t_certainty[certainty_index] << "% certainty.\n";

  for (unsigned int event = 0; event < PAPI_max_event; ++event)
  {
    size_t n = count_array[event].size();
    if (n > 0)
    {
      stats_st stats = determine_stats(certainty_index, event);
      n = stats.n;
      if (n == 0)
        continue;
      cout << setw(18) << PAPI_event_name(event) << " (" << setw(4) << n << ") : " << setw(10);
      cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
      cout << setprecision(0) << stats.x_avg << " +/- ";
      cout.width(10);
      cout.setf(std::ios_base::left, std::ios_base::adjustfield);
      if (n == 1)
        cout << "?\n";
      else
      {
	double err = t(certainty_index, n - 1) * stats.s_n1 / sqrt(n);
	cout << /*(long long)*/err;
        cout.setf(std::ios_base::right, std::ios_base::adjustfield);
        cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
	cout.precision(1);
	cout << '(' << setw(4) << 100.0 * err / stats.x_avg << " % )\n";
      }
      
      if (_G_plot)
      {
	int np = 200;
	if (stats.xright - stats.xleft + 1 <= np)
	  np = (int)(stats.xright - stats.xleft + 1);
	float* xa = new float [np];
	float* ya = new float [np];
	for (int i = 0; i < np; ++i)
	{
	  ya[i] = 0;
	  xa[i] = stats.xleft + i * (stats.xright - stats.xleft) / (np - 1);
	}
	for (std::vector<data_ut>::iterator iter(count_array[event].begin()); iter != count_array[event].end(); ++iter)
	{
	  double count = (*iter).i_data;
	  if (count > stats.cut_off_high || count < stats.cut_off_low)
	    continue;
	  size_t i = (size_t)((np - 1) * (count - stats.xleft) / (stats.xright - stats.xleft) + 0.5);
	  ya[i]++;
	}
	//for (int i = 0; i < np; ++i)
	//  cout << "x, y = " << xa[i] << ", " << ya[i] << endl;
	std::string filename(PAPI_event_name(event));
	filename += ".xg";
	ofstream xgdata;
	xgdata.open(filename.c_str());
	xgdata << "TitleFont: fixed-14\n";
	xgdata << "TitleText: " << header << '\n';
	xgdata << "Device: Postscript\n";
	xgdata << "Disposition: To File\n";
	xgdata << "FileOrDev: " << PAPI_event_name(event) << ".ps\n";
	xgdata << "Markers: On\n";
	//xgdata << "0.Color: blue\n";
	xgdata << "XLowLimit: " << 0 << '\n';
	//xgdata << "XHighLimit: " << xright << '\n';
	xgdata << "YUnitText: Count\n";
	xgdata << "XUnitText: " << PAPI_event_name(event) << '\n';
	xgdata << '\"' << PAPI_event_name(event) << " \"\n";
	for (int i = 0; i < np; ++i)
	  xgdata << xa[i] << ' ' << ya[i] << '\n';
	xgdata.close();
	delete [] xa;
	delete [] ya;
      }
    }
  }
  for (unsigned int event = 0; event < PAPI_max_event; ++event)
  {
    if (count_array[event].size() > 0)
      count_array[event].erase(count_array[event].begin(), count_array[event].end());
  }
}
