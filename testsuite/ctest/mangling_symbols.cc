// The test keeps representative free functions, member functions, pointer-to-member
// parameters, nested scope names, constructors, destructors, virtual functions,
// and variables in this translation unit, then compares libcwd's demangler with
// the platform C++ ABI demangler for each symbol.
//
// Symbols whose code or data address can be taken directly are resolved through
// pc_mangled_function_name; constructors, destructors, and ordinary member functions
// are covered by their Itanium ABI mangled names because C++ does not provide a
// portable object-code address for those declarations.

#include "cwd_sys.h"
#include "test_support.h"

#include <cxxabi.h>
#include <libcwd/demangle.h>
#include <libcwd/pc_mangled_function_name.h>
#include <libcwd/type_info.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

struct MyAcceptedSocketDecoder { };

namespace evio {

struct OutputStream { };

template<typename T1, typename T2>
struct AcceptedSocket { };

} // namespace evio

void nonscopetype_void() { }
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

struct class_name {
  void scopetype_class_name();
  void scopetype_class_name_const() const;
};

void class_name::scopetype_class_name() { }
void class_name::scopetype_class_name_const() const { }
void scopetype_member_function_pointer_example_class_name(return_type (class_name::*)(parameterI, parameterII, parameterIII)) { }
void scopetype_member_function_pointer_const_example_class_name(return_type (class_name::* const)(parameterI, parameterII, parameterIII)) { }
void scopetype_const_member_function_pointer_example_class_name(return_type (class_name::*)(parameterI, parameterII, parameterIII) const) { }
void scopetype_const_member_function_pointer_const_example_class_name(return_type (class_name::* const)(parameterI, parameterII, parameterIII) const) { }

template<typename ttypeI, typename ttypeII, typename ttypeIII>
struct template_name {
  void scopetype_template_name_ttypes();
  void scopetype_template_name_ttypes_const() const;
};

template<typename ttypeI, typename ttypeII, typename ttypeIII>
void template_name<ttypeI, ttypeII, ttypeIII>::scopetype_template_name_ttypes() { }

template<typename ttypeI, typename ttypeII, typename ttypeIII>
void template_name<ttypeI, ttypeII, ttypeIII>::scopetype_template_name_ttypes_const() const { }

void scopetype_member_function_pointer_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::*)(parameterI, parameterII, parameterIII)) { }
void scopetype_member_function_pointer_const_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::* const)(parameterI, parameterII, parameterIII)) { }
void scopetype_const_member_function_pointer_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::*)(parameterI, parameterII, parameterIII) const) { }
void scopetype_const_member_function_pointer_const_example_template_name_ttypes(return_type (template_name<ttypeI, ttypeII, ttypeIII>::* const)(parameterI, parameterII, parameterIII) const) { }

scopetype const st = { };
template<class ScopetypeConst> void possiblescopetype_const(ScopetypeConst&) { }

struct scopetypeI {
  struct scopetypeII {
    struct scopetypeIII {
      class type { };
    };
  };
};

void possiblescopetype_scopetypes_type(scopetypeI::scopetypeII::scopetypeIII::type) { }

int symbol_int;
int symbol_func(parameterI, parameterII, parameterIII) { return 0; }

struct scopetypeIV {
  struct symbol_constructor_void {
    symbol_constructor_void();
    ~symbol_constructor_void();
  };
  struct symbol_constructor_types {
    symbol_constructor_types(parameterI, parameterII, parameterIII);
    ~symbol_constructor_types();
  };
  virtual void virtual_func();
  static int symbol_scopetype_symbol;
};

scopetypeIV::symbol_constructor_void::symbol_constructor_void() { }
scopetypeIV::symbol_constructor_types::symbol_constructor_types(parameterI, parameterII, parameterIII) { }
scopetypeIV::symbol_constructor_void::~symbol_constructor_void() { }
scopetypeIV::symbol_constructor_types::~symbol_constructor_types() { }
void scopetypeIV::virtual_func() { }
int scopetypeIV::symbol_scopetype_symbol;

namespace {

std::string modern_demangle(char const* mangled_name)
{
  int status = 0;
  std::unique_ptr<char, decltype(&std::free)> demangled(
      abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status), &std::free);
  if (status == 0 && demangled)
    return demangled.get();
  return mangled_name ? mangled_name : "(null)";
}

bool compare_mangled_name(char const* case_name, char const* mangled_name)
{
  std::string libcwd_result;
  libcwd::demangle_symbol(mangled_name, libcwd_result);
  std::string modern_result = modern_demangle(mangled_name);
  if (libcwd_result != modern_result)
  {
    std::cerr << case_name << ": demangler mismatch for `" << mangled_name << "'\n"
              << "  libcwd: " << libcwd_result << "\n"
              << "  modern: " << modern_result << "\n";
    return false;
  }
  return true;
}

bool compare_address(char const* case_name, void const* ptr)
{
  char const* mangled_name = libcwd::pc_mangled_function_name(ptr);
  return compare_mangled_name(case_name, mangled_name);
}

// Compare libcwd's type_info_of result with the platform ABI demangler for the
// base object type plus an optional exact-type suffix. The suffix accounts for
// top-level qualifiers and references because typeid(T).name() does not encode
// those for the platform demangler, while libcwd::type_info_of<T>() deliberately
// preserves them.
template<typename ExactType, typename ObjectType>
bool compare_type_info(char const* case_name, ObjectType const& object, char const* exact_type_suffix = "")
{
  std::string libcwd_type = libcwd::type_info_of<ExactType>().demangled_name();
  std::string modern_type = modern_demangle(typeid(object).name()) + exact_type_suffix;
  if (libcwd_type != modern_type)
  {
    std::cerr << case_name << ": type_info_of mismatch\n"
              << "  libcwd:   " << libcwd_type << "\n"
              << "  expected: " << modern_type << "\n";
    return false;
  }
  return true;
}

} // namespace

int main()
{
  Debug(main_reached());

  bool success = true;

#define CHECK_ADDRESS(symbol) success = compare_address(#symbol, reinterpret_cast<void const*>(symbol)) && success
  CHECK_ADDRESS(nonscopetype_void);
  CHECK_ADDRESS(nonscopetype_ellipsis);
  CHECK_ADDRESS(nonscopetype_bool);
  CHECK_ADDRESS(nonscopetype_float);
  CHECK_ADDRESS(nonscopetype_double);
  CHECK_ADDRESS(nonscopetype_long_double);
  CHECK_ADDRESS(nonscopetype_wchar_t);
  CHECK_ADDRESS(nonscopetype_signed_char);
  CHECK_ADDRESS(nonscopetype_unsigned_char);
  CHECK_ADDRESS(nonscopetype_char);
  CHECK_ADDRESS(nonscopetype_unsigned_short);
  CHECK_ADDRESS(nonscopetype_short);
  CHECK_ADDRESS(nonscopetype_unsigned);
  CHECK_ADDRESS(nonscopetype_int);
  CHECK_ADDRESS(nonscopetype_unsigned_long);
  CHECK_ADDRESS(nonscopetype_long);
  CHECK_ADDRESS(nonscopetype_unsigned_long_long);
  CHECK_ADDRESS(nonscopetype_long_long);
  CHECK_ADDRESS(nonscopetype_pointer);
  CHECK_ADDRESS(nonscopetype_reference);
  CHECK_ADDRESS(nonscopetype_function_pointer);
  CHECK_ADDRESS(nonscopetype_function_pointer_const);
  CHECK_ADDRESS(nonscopetype_function_reference);
  CHECK_ADDRESS(nonscopetype_member_function_pointer);
  CHECK_ADDRESS(nonscopetype_member_function_pointer_const);
  CHECK_ADDRESS(nonscopetype_const_member_function_pointer);
  CHECK_ADDRESS(nonscopetype_const_member_function_pointer_const);
  CHECK_ADDRESS(scopetype_member_function_pointer_example_class_name);
  CHECK_ADDRESS(scopetype_member_function_pointer_const_example_class_name);
  CHECK_ADDRESS(scopetype_const_member_function_pointer_example_class_name);
  CHECK_ADDRESS(scopetype_const_member_function_pointer_const_example_class_name);
  CHECK_ADDRESS(scopetype_member_function_pointer_example_template_name_ttypes);
  CHECK_ADDRESS(scopetype_member_function_pointer_const_example_template_name_ttypes);
  CHECK_ADDRESS(scopetype_const_member_function_pointer_example_template_name_ttypes);
  CHECK_ADDRESS(scopetype_const_member_function_pointer_const_example_template_name_ttypes);
  CHECK_ADDRESS(possiblescopetype_scopetypes_type);
  CHECK_ADDRESS(symbol_func);
#undef CHECK_ADDRESS

  success = compare_address("symbol_int", &symbol_int) && success;

  char const* explicit_mangled_names[] = {
    "_ZN10class_name20scopetype_class_nameEv",
    "_ZNK10class_name26scopetype_class_name_constEv",
    "_ZN11scopetypeIV23symbol_constructor_voidC1Ev",
    "_ZN11scopetypeIV24symbol_constructor_typesC1E10parameterI11parameterII12parameterIII",
    "_ZN11scopetypeIV23symbol_constructor_voidD1Ev",
    "_ZN11scopetypeIV24symbol_constructor_typesD1Ev",
    "_ZN11scopetypeIV12virtual_funcEv",
    "_ZN11scopetypeIV23symbol_scopetype_symbolE",
    nullptr
  };
  for (char const** mangled_name = explicit_mangled_names; *mangled_name; ++mangled_name)
    success = compare_mangled_name(*mangled_name, *mangled_name) && success;

  template_name<ttypeI, ttypeII, ttypeIII> instantiate;
  evio::AcceptedSocket<MyAcceptedSocketDecoder, evio::OutputStream> accepted_socket;

#define CHECK_TYPE_INFO(case_name, exact_type, object, suffix) success = compare_type_info<exact_type>(case_name, object, suffix) && success
  CHECK_TYPE_INFO("type_info_of(template_name)", decltype(instantiate), instantiate, "");
  CHECK_TYPE_INFO("type_info_of(evio::AcceptedSocket const&)", decltype(accepted_socket) const&, accepted_socket, " const&");
#undef CHECK_TYPE_INFO

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
