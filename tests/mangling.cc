// Used helper types.
class prefix { };
class return_type { };
class parameterI { };
class parameterII { };
class parameterIII { };
struct scopetype {
  return_type f1(parameterI, parameterII, parameterIII) { }
  return_type f2(parameterI, parameterII, parameterIII) const { }
};
class ttypeI { };
class ttypeII { };
class ttypeIII { };

// A symbol.
int symbol;			// <symbol>				--> <symbol>

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
void nonscopetype_function_reference(return_type (&)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_member_function_pointer(return_type (scopetype::*foo)(parameterI, parameterII, parameterIII)) { }
void nonscopetype_const_member_function_pointer(return_type (scopetype::*foo)(parameterI, parameterII, parameterIII) const) { }

// Scope types.
struct class_name {
  void scopetype_class_name(void);
};
void class_name::scopetype_class_name(void) { }

template<typename ttypeI, typename ttypeII, typename ttypeIII>
  struct template_name {
    void scopetype_template_name_ttypes(void);
  };
template<typename ttypeI, typename ttypeII, typename ttypeIII>
  void template_name<ttypeI, ttypeII, ttypeIII>::scopetype_template_name_ttypes(void) { }
template_name<ttypeI, ttypeII, ttypeIII> instantiate;

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

// Template instantiation
int main(void)
{
  template_name<ttypeI, ttypeII, ttypeIII> instantiate;
  instantiate.scopetype_template_name_ttypes();
  possiblescopetype_const(st);
  return 0;
}
