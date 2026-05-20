#pragma once

#include <elfutils/libdw.h>

namespace libcwd {

class object_file_ct;

namespace dwarf {

struct symbol_ct_key
{
 protected:
  Dwarf_Addr real_start_;
  Dwarf_Addr real_end_;

 public:
  // To be used as search key.
  symbol_ct_key(Dwarf_Addr start) : real_start_(start), real_end_(start + 1) { }

 protected:
  symbol_ct_key(Dwarf_Addr real_start, Dwarf_Addr real_end) : real_start_(real_start), real_end_(real_end) { }

 public:
  Dwarf_Addr real_start() const { return real_start_; }
  Dwarf_Addr real_end() const { return real_end_; }
};

struct symbol_ct_interface : public symbol_ct_key
{
  //symbol_ct const* objfile_ct::find_symbol(symbol_ct const& search_key) const
  symbol_ct_interface(Dwarf_Addr real_start, Dwarf_Addr real_end) : symbol_ct_key(real_start, real_end) { }

  virtual char const* name() const = 0;
  virtual bool diecu(Dwarf_Die* cu_die_out) const = 0;
  virtual ~symbol_ct_interface() = default;
};

struct objfiles_ct_interface
{
  virtual object_file_ct const* get_object_file() const = 0;
  virtual uintptr_t get_lbase() const = 0;
  virtual symbol_ct_interface const* find_symbol(symbol_ct_key const& search_key) const = 0;
  virtual ~objfiles_ct_interface() = default;
};

} // namespace dwarf
} // namespace libcwd
