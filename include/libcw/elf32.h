namespace libcw {
  namespace debug {

namespace cwbfd {
class object_file_ct;
}

namespace elf32 {

// Figure 1-2: 32-Bit Data Types.
 
typedef u_int32_t       Elf32_Addr;             // Unsigned program address.
typedef u_int16_t       Elf32_Half;             // Unsigned medium integer.
typedef u_int32_t       Elf32_Off;              // Unsigned file offset.
typedef int32_t         Elf32_Sword;            // Signed large integer.
typedef u_int32_t       Elf32_Word;             // Unsigned large integer.

struct asection_st;
struct bfd_st;

struct udata_st {
  size_t p;
};

struct asymbol_st {
  bfd_st* bfd_ptr;
  asection_st* section;
  Elf32_Off value;
  udata_st udata;
  Elf32_Word flags;
  char const* name;
};

struct asection_st {
  Elf32_Addr vma;
  char const* name;
};

struct bfd_st {
  union {
    char* usrdata;
    cwbfd::object_file_ct const* object_file;
  };
  bool cacheable;
  virtual ~bfd_st() { }
public:
  static bfd_st* openr(char const* filename);
  virtual bool check_format(void) const = 0;
  virtual long get_symtab_upper_bound(void) = 0;
  virtual long canonicalize_symtab(asymbol_st**) = 0;
  virtual void find_nearest_line(asection_st*, asymbol_st**, Elf32_Addr, char const**, char const**, unsigned int*) const = 0;
};

} // namespace elf32

  } // namespace debug
} // namespace libcw
