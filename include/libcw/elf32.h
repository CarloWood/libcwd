namespace libcw {
  namespace debug {

namespace cwbfd {

static int const BSF_LOCAL             = 0x1;
static int const BSF_GLOBAL            = 0x2;
static int const BSF_DEBUGGING         = 0x8;
static int const BSF_FUNCTION          = 0x10;
static int const BSF_WEAK              = 0x80;
static int const BSF_SECTION_SYM       = 0x100;
static int const BSF_OLD_COMMON        = 0x200;
static int const BSF_NOT_AT_END        = 0x400;
static int const BSF_CONSTRUCTOR       = 0x800;
static int const BSF_WARNING           = 0x1000;
static int const BSF_INDIRECT          = 0x2000;
static int const BSF_FILE              = 0x4000;
static int const BSF_DYNAMIC           = 0x8000;
static int const BSF_OBJECT            = 0x10000;

class object_file_ct;

} // namespace cwbfd

namespace elf32 {

// Figure 1-2: 32-Bit Data Types.
 
typedef unsigned int32_t	Elf32_Addr;             // Unsigned program address.
typedef unsigned int16_t	Elf32_Half;             // Unsigned medium integer.
typedef unsigned int32_t	Elf32_Off;              // Unsigned file offset.
typedef          int32_t	Elf32_Sword;            // Signed large integer.
typedef unsigned int32_t	Elf32_Word;             // Unsigned large integer.

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
  char const* filename;
  union {
    char* usrdata;
    cwbfd::object_file_ct const* object_file;
  };
  bool cacheable;
  bool M_has_syms;
public:
  bfd_st(void) : M_has_syms(false) { }
  virtual ~bfd_st() { }
public:
  static bfd_st* openr(char const* file_name);
  virtual bool check_format(void) const = 0;
  virtual long get_symtab_upper_bound(void) = 0;
  virtual long canonicalize_symtab(asymbol_st**) = 0;
  virtual void find_nearest_line(asymbol_st const*, Elf32_Addr, char const**, char const**, unsigned int*) = 0;
  bool has_syms(void) const { return M_has_syms; }
};

} // namespace elf32

  } // namespace debug
} // namespace libcw
