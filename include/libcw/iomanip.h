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

#ifndef LIBCW_IOMANIP_H
#define LIBCW_IOMANIP_H

RCSTAG_H(iomanip, "$Id$")

#include <iosfwd>
#include <vector>
#include <algo.h>

template<class OMANIP_DATA>
class omanip_id_tct {
  OMANIP_DATA omanip_data;
  class ios const* id;
public:
  omanip_id_tct(class ios const* iosp) : omanip_data(), id(iosp) { }
  bool operator==(class ios const* iosp) const { return id == iosp; }
  OMANIP_DATA const& get_omanip_data(void) const { return omanip_data; }
  OMANIP_DATA& get_omanip_data(void) { return omanip_data; }
};

template<class IMANIP_DATA>
class imanip_id_tct {
  IMANIP_DATA imanip_data;
  class ios const* id;
public:
  imanip_id_tct(class ios const* iosp) : imanip_data(), id(iosp) { }
  bool operator==(class ios const* iosp) const { return id == iosp; }
  IMANIP_DATA const& get_imanip_data(void) const { return imanip_data; }
  IMANIP_DATA& get_imanip_data(void) { return imanip_data; }
};

namespace {
  template<class TYPE>
    struct compiler_bug_workaround {
      static TYPE ids;
    };

  template<class TYPE>
  TYPE compiler_bug_workaround<TYPE>::ids;
}

template<class TYPE>
inline typename TYPE::omanip_data_ct& get_omanip_data(ostream const& os)
{
  typedef omanip_id_tct<typename TYPE::omanip_data_ct> omanip_id_ct;
  typedef vector<omanip_id_ct> ids_ct;
  ids_ct& ids(compiler_bug_workaround<ids_ct>::ids);	// static ids_ct ids;
  typename ids_ct::iterator i = find(ids.begin(), ids.end(), &os);
  if (i == ids.end())
    i = ids.insert(ids.end(), omanip_id_ct(&os));
  return (*i).get_omanip_data();
}

template<class TYPE>
#ifdef DWARF2OUT_BUG
static
#else
inline
#endif
typename TYPE::imanip_data_ct& get_imanip_data(istream const& os)
{
  typedef imanip_id_tct<typename TYPE::imanip_data_ct> imanip_id_ct;
  typedef vector<imanip_id_ct> ids_ct;
  static ids_ct ids;
  typename ids_ct::iterator i = find(ids.begin(), ids.end(), &os);
  if (i == ids.end())
    i = ids.insert(ids.end(), imanip_id_ct(&os));
  return (*i).get_imanip_data();
}

// Manipulator definitions.

#define __DEFINE_OMANIP0(TYPE, function) \
  enum __omanip0_##function { function }; \
  friend inline ostream& operator<<(ostream& os, __omanip0_##function) \
      { (get_omanip_data<TYPE>(os)).function(); return os; }

#define __DEFINE_IMANIP0(TYPE, function) \
  enum __imanip0_##function { function }; \
  friend inline istream& operator>>(istream& is, __imanip0_##function) \
      { (get_imanip_data<TYPE>(is)).function(); return is; }

template <class FT, class TP1> class manip1_tct {
public:
  FT func;
  TP1 param1;
  manip1_tct(FT f, TP1 p1) : func(f), param1(p1) { }
};

#define __DEFINE_OMANIP1_OPERATOR(TYPE, TP1) \
  friend inline ostream& operator<<(ostream& os, manip1_tct<void (omanip_data_ct::*)(TP1), TP1> const& m) \
      { (get_omanip_data<TYPE>(os).*m.func)(m.param1); return os; }

#define __DEFINE_OMANIP1_FUNCTION(function, TP1) \
  static inline manip1_tct<void (omanip_data_ct::*)(TP1), TP1> function(TP1 p1) \
      { return manip1_tct<void (omanip_data_ct::*)(TP1), TP1>(&omanip_data_ct::function, p1); }

#define __DEFINE_IMANIP1_OPERATOR(TYPE, TP1) \
  friend inline istream& operator>>(istream& is, manip1_tct<void (imanip_data_ct::*)(TP1), TP1> const& m) \
      { (get_imanip_data<TYPE>(is).*m.func)(m.param1); return is; }

#define __DEFINE_IMANIP1_FUNCTION(function, TP1) \
  static inline manip1_tct<void (imanip_data_ct::*)(TP1), TP1> function(TP1 p1) \
      { return manip1_tct<void (imanip_data_ct::*)(TP1), TP1>(&imanip_data_ct::function, p1); }

template <class FT, class TP1, class TP2> class manip2_tct {
public:
  FT func;
  TP1 param1;
  TP2 param2;
  manip2_tct(FT f, TP1 p1, TP2 p2) : func(f), param1(p1), param2(p2) { }
};

#define __DEFINE_OMANIP2_OPERATOR(TYPE, TP1, TP2) \
  friend inline ostream& operator<<(ostream& os, manip2_tct<void (omanip_data_ct::*)(TP1, TP2), TP1, TP2> const& m) \
      { (get_omanip_data<TYPE>(os).*m.func)(m.param1, m.param2); return os; }

#define __DEFINE_OMANIP2_FUNCTION(function, TP1, TP2) \
  static inline manip2_tct<void (omanip_data_ct::*)(TP1, TP2), TP1, TP2> function(TP1 p1, TP2 p2) \
      { return manip2_tct<void (omanip_data_ct::*)(TP1, TP2), TP1, TP2>(&omanip_data_ct::function, p1, p2); }

#define __DEFINE_IMANIP2_OPERATOR(TYPE, TP1, TP2) \
  friend inline istream& operator>>(istream& is, manip2_tct<void (imanip_data_ct::*)(TP1, TP2), TP1, TP2> const& m) \
      { (get_imanip_data<TYPE>(is).*m.func)(m.param1, m.param2); return is; }

#define __DEFINE_IMANIP2_FUNCTION(function, TP1, TP2) \
  static inline manip2_tct<void (imanip_data_ct::*)(TP1, TP2), TP1, TP2> function(TP1 p1, TP2 p2) \
      { return manip2_tct<void (imanip_data_ct::*)(TP1, TP2), TP1, TP2>(&imanip_data_ct::function, p1, p2); }

#endif // LIBCW_IOMANIP_H
