#include "sys.h"
#include <iostream>
#include <libcw/bfd.h>
#include <libcw/type_info.h>
#include <libcw/demangle.h>

// Used helper types.
class prefix { };
class return_type { } return_type_instance;
class parameterI { };
class parameterII { };
class parameterIII { };
struct scopetype {
  return_type symbol_scopetype_func(parameterI, parameterII, parameterIII);
  return_type symbol_scopetype_func_const(parameterI, parameterII, parameterIII) const;
};
return_type scopetype::symbol_scopetype_func(parameterI, parameterII, parameterIII) { return return_type_instance; }
return_type scopetype::symbol_scopetype_func_const(parameterI, parameterII, parameterIII) const { return return_type_instance; }
class ttypeI { };
class ttypeII { };
class ttypeIII { };

// Non-scope parameters (as parameter of nonscopetype_*).
void nonscopetype_void(void) { }
void nonscopetype_ellipsis(...) { }
void nonscopetype_bool(bool) { }
void nonscopetype_float(float) { }
void nonscopetype_double(double) { }
void nonscopetype_long_double(long double) { }
void nonscopetype_wchar_t(wchar_t) { }
void nonscopetype_signed_char(signed char) { }
void nonscopetype_unsigned_char(unsigned char) { }
void nonscopetype_char(char) { }
void nonscopetype_unsigned_short(unsigned short) { }
void nonscopetype_short(short) { }
void nonscopetype_unsigned(unsigned int) { }
void nonscopetype_int(int) { }
void nonscopetype_unsigned_long(unsigned long) { }
void nonscopetype_long(long) { }
void nonscopetype_unsigned_long_long(unsigned long long) { }
void nonscopetype_long_long(long long) { }
void nonscopetype_pointer(prefix*) { }
void nonscopetype_reference(prefix&) { }
void nonscopetype_function_pointer(return_type (*)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_function_pointer_const(return_type (* const)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_function_reference(return_type (&)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_member_function_pointer(return_type (scopetype::*)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_member_function_pointer_const(return_type (scopetype::* const)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_const_member_function_pointer(return_type (scopetype::*)(parameterI, parameterII, parameterIII) const) { }
void nonscopetype_const_member_function_pointer_const(return_type (scopetype::* const)(parameterI, parameterII, parameterIII) const) { }

// Scope types.
struct class_name {
  void scopetype_class_name(void);
  void scopetype_class_name_const(void) const;
};
void class_name::scopetype_class_name(void) { }
void class_name::scopetype_class_name_const(void) const { }
void scopetype_member_function_pointer_example_class_name(return_type (class_name::*)(parameterI, parameterII, parameterIII)) { }
void scopetype_member_function_pointer_const_example_class_name(return_type (class_name::* const)(parameterI, parameterII, parameterIII)) { }
void scopetype_const_member_function_pointer_example_class_name(return_type (class_name::*)(parameterI, parameterII, parameterIII) const) { }
void scopetype_const_member_function_pointer_const_example_class_name(return_type (class_name::* const)(parameterI, parameterII, parameterIII) const) { }

template<typename ttypeI, typename ttypeII, typename ttypeIII>
  struct template_name {
    void scopetype_template_name_ttypes(void);
    void scopetype_template_name_ttypes_const(void) const;
  };
template<typename ttypeI, typename ttypeII, typename ttypeIII>
  void template_name<ttypeI, ttypeII, ttypeIII>::scopetype_template_name_ttypes(void) { }
template<typename ttypeI, typename ttypeII, typename ttypeIII>
  void template_name<ttypeI, ttypeII, ttypeIII>::scopetype_template_name_ttypes_const(void) const { }
void scopetype_member_function_pointer_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::*)(parameterI, parameterII, parameterIII)) { }
void scopetype_member_function_pointer_const_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::* const)(parameterI, parameterII, parameterIII)) { }
void scopetype_const_member_function_pointer_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::*)(parameterI, parameterII, parameterIII) const) { }
void scopetype_const_member_function_pointer_const_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::* const)(parameterI, parameterII, parameterIII) const) { }

// Possible scope types
scopetype const st = { };
template<class ScopetypeConst> void possiblescopetype_const(ScopetypeConst& foo) { }

struct scopetypeI {
  struct scopetypeII {
    struct scopetypeIII {
      class type { };
    };
  };
};
void possiblescopetype_scopetypes_type(scopetypeI::scopetypeII::scopetypeIII::type) { }

// Symbols

int symbol_int;
int symbol_func(parameterI, parameterII, parameterIII) { return 0; }

struct scopetypeIV {
  struct symbol_constructor_void {
    symbol_constructor_void(void);
    ~symbol_constructor_void();
  };
  struct symbol_constructor_types {
    symbol_constructor_types(parameterI, parameterII, parameterIII);
    ~symbol_constructor_types();
  };
  virtual void virtual_func(void);
  static int symbol_scopetype_symbol;
};
scopetypeIV::symbol_constructor_void::symbol_constructor_void(void) { }
scopetypeIV::symbol_constructor_types::symbol_constructor_types(parameterI, parameterII, parameterIII) { }
scopetypeIV::symbol_constructor_void::~symbol_constructor_void() { }
scopetypeIV::symbol_constructor_types::~symbol_constructor_types() { }
void scopetypeIV::virtual_func(void) { }
int scopetypeIV::symbol_scopetype_symbol;

// Template instantiation
int main(void)
{
  template_name<ttypeI, ttypeII, ttypeIII> instantiate;
  instantiate.scopetype_template_name_ttypes();
  possiblescopetype_const(st);

  void* ptr = (void*)nonscopetype_void;
  std::string result;
  libcw::debug::demangle_symbol(libcw::debug::pc_mangled_function_name(ptr), result);
  std::cout << result << '\n';
  std::cout << libcw::debug::type_info_of(instantiate).demangled_name() << '\n';
  return 0;
}

namespace {
  int anonymous;
}

