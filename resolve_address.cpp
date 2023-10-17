#define UNW_LOCAL_ONLY
#include <iostream>
#include <elfutils/libdw.h>
#include <fcntl.h>
#include <dwarf.h>
#include <unistd.h>
#include <libunwind.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <vector>

// Compile as:
//
// g++ -g -o resolve_address resolve_address.cpp -lunwind -ldw -lelf
//

#ifdef WRITE_PROC_MAPS
void writeProcMapsToFile();
#endif

std::string read_build_id(std::string const& object_file)
{
  // Determine the working version (the ELF version supported by both, the libelf library and this program).
  if (elf_version(EV_CURRENT) == EV_NONE)
  {
    std::cerr << "Warning: libelf is out of date. Can't read build-id of \"" << object_file << "\" (or any object file)." << std::endl;
  }
  else
  {
    // Open the ELF object file.
    int fd = open(object_file.c_str(), O_RDONLY);
    if (fd == -1)
    {
      std::cerr << "Warning: failed to open file " << object_file << std::endl;
    }
    else
    {
      // Open an ELF descriptor for reading.
      Elf* e = elf_begin(fd, ELF_C_READ, NULL);
      if (!e)
      {
        std::cerr << "Warning: elf_begin returned NULL for \"" << object_file << "\": " << elf_errmsg(-1) << std::endl;
      }
      else
      {
        if (elf_kind(e) != ELF_K_ELF)
        {
          std::cout << object_file << ": skipping, not an ELF file." << std::endl;
        }
        else
        {
#if 0
          // Get the string table section index.
          size_t shstrndx;
          if (elf_getshdrstrndx(e, &shstrndx) != 0)
          {
            std::cerr << "elf_getshdrstrndx() failed: " << elf_errmsg(-1) << std::endl;
            return "";
          }
#endif
          // Run over all sections in the ELF file.
          Elf_Scn* scn = NULL;
          while ((scn = elf_nextscn(e, scn)) != NULL)
          {
            // Get the section header.
            GElf_Shdr shdr;
            gelf_getshdr(scn, &shdr);
            if (shdr.sh_type == SHT_NOTE)
            {
#if 0
              // Get the name of the section
              char const* name = elf_strptr(e, shstrndx, shdr.sh_name);
              std::cout << "Section: " << name << std::endl;
#endif
              Elf_Data* data = elf_getdata(scn, NULL);
              GElf_Nhdr nhdr;
              size_t name_offset, desc_offset, offset = 0;
              while ((offset = gelf_getnote(data, offset, &nhdr, &name_offset, &desc_offset)) > 0)
              {
                if (nhdr.n_type == NT_GNU_BUILD_ID && nhdr.n_namesz == 4 && strncmp((char*)data->d_buf + name_offset, "GNU", 4) == 0)
                {
                  unsigned char *desc = (unsigned char *)data->d_buf + desc_offset;
                  std::stringstream oss;
                  for (int i = 0; i < nhdr.n_descsz; ++i)
                    oss << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)desc[i];
                  //std::cout << "build-id of " << object_file << " is " << oss.str() << std::endl;
                  return oss.str();
                }
              }
            }
          }
        }
        elf_end(e);
      }
      close(fd);
    }
  }
  return {};
}

std::string get_debug_symbols_path(std::string object_file)
{
  std::string debug_symbols_path;

  char const* build_id_dir = "/usr/lib/debug/.build-id";
  struct stat sb;
  if (stat(build_id_dir, &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    // Get the build-id, if any.
    std::string build_id = read_build_id(object_file);
    if (build_id.length() > 2)
    {
      debug_symbols_path = build_id_dir;
      debug_symbols_path += '/';
      debug_symbols_path += build_id.substr(0, 2);
      debug_symbols_path += '/';
      debug_symbols_path += build_id.substr(2);
      debug_symbols_path += ".debug";
    }
  }
  else
    debug_symbols_path = "/usr/lib/debug" + object_file + ".debug";
  if (stat(debug_symbols_path.c_str(), &sb) != 0)
    debug_symbols_path = object_file;
  std::cout << "get_debug_symbols_path returns: \"" << debug_symbols_path << "\"." << std::endl;
  return debug_symbols_path;
}

void resolve_address(void const* addr)
{
  //===========================================================================
  // Find the object file name
  //
  Dl_info info;
  if (!dladdr(addr, &info))
  {
    std::cerr << "dladdr: " << dlerror() << "\n";
    return;
  }

  char* object_file_name = canonicalize_file_name(info.dli_fname);
  if (!object_file_name)
  {
    std::cerr << "canonicalize_file_name" << std::strerror(errno) << std::endl;
    return;
  }

  std::cout << "Object file: \"" << object_file_name << "\"\n";
  //
  //===========================================================================

#if 0
  // dli_sname is only available for dynamically exported symbols,
  // and even then dladdr might still not be able to resolve it.
  // Therefore dli_sname is not really useful.
  if (info.dli_sname)
    std::cout << "dli_sname: \"" << info.dli_sname << "\"\n";
#endif

  //===========================================================================
  // Open (corresponding) file with DWARF debug info.
  //
  int fd = open(get_debug_symbols_path(object_file_name).c_str(), O_RDONLY);
  free(object_file_name);
  if (fd < 0)
  {
    std::cerr << "Failed to open ELF file.\n";
    return;
  }

  Dwarf* dwarf = dwarf_begin(fd, DWARF_C_READ);
  if (!dwarf)
  {
    std::cerr << "Failed to initialize libdw. Error: " << dwarf_errmsg(-1) << '\n';
    close(fd);
    return;
  }
  //
  //===========================================================================

  // Get the DIE (Debugging Information Entry) for the function.

  // libdw works with addresses relative to the load address of an object file.
  // This load, or base address can be found in info.dli_fbase.
  Dwarf_Addr rel_ip = (Dwarf_Addr)(addr) - (Dwarf_Addr)(info.dli_fbase);

  printf("lbase = %lx; addr = %lx; rel_ip = %lx.\n", (Dwarf_Addr)info.dli_fbase, (Dwarf_Addr)addr, rel_ip);

  //===========================================================================
  // Iterate over all compilation units of the given object file.
  //
  Dwarf_Off offset = 0;
  Dwarf_Off next_offset;
  size_t hsize;
  bool found_die = false;
  while (dwarf_nextcu(dwarf, offset, &next_offset, &hsize, nullptr, nullptr, nullptr) == 0)
  {
    std::cout << "Compilation unit at offset: 0x" << std::hex << offset << std::dec << std::endl;
    std::cout << "  Header size: " << hsize << std::endl;

    //=========================================================================
    // Find the compilation unit Debug Information Entry (DIE).
    //
    Dwarf_Die cu_die;
    Dwarf_Die* cu_die_ptr = dwarf_offdie(dwarf, offset + hsize, &cu_die);
    if (cu_die_ptr)
    {
      assert(cu_die_ptr == &cu_die);

      //=======================================================================
      // Compilation unit name
      //
      char const* name = dwarf_diename(cu_die_ptr);
      if (name)
        printf("Found compilation unit: %s\n", name);
      else
        printf("DIE name not found\n");

      Dwarf_Attribute attr;
      if (dwarf_attr(cu_die_ptr, DW_AT_name, &attr) != NULL)
      {
        char const* cu_name = dwarf_formstring(&attr);
        if (cu_name)
          std::cout << "Compilation unit DW_AT_name: " << cu_name << std::endl;
      }
      else
        std::cout << "dwarf_attr returned NULL" << std::endl;

      //=======================================================================
      // Check if this compilation unit has an address range that contains rel_ip.
      //

      bool found_compilation_unit = false;
      Dwarf_Addr lbase;
      Dwarf_Addr start;
      Dwarf_Addr end;
      ptrdiff_t offset = 0;
      while ((offset = dwarf_ranges(cu_die_ptr, offset, &lbase, &start, &end)) > 0)
      {
        if (end != (Dwarf_Addr)-1)
        {
          std::cout << std::hex << "start = 0x" << start << "; end = 0x" << end;
          if (start <= rel_ip && rel_ip < end)
          {
            std::cout << " -- FOUND!" << std::endl;
            found_compilation_unit = true;
            break;
          }
          std::cout << std::endl;
        }
      }

      // The correct compilation unit?
      if (found_compilation_unit)
      {
        //=====================================================================
        // Iterate over all children of the compilation unit.
        //

        // Get the first child of this DIE.
        Dwarf_Die child_die;
        if (dwarf_child(cu_die_ptr, &child_die) != 0)
        {
          std::cerr << "dwarf_child failed" << std::endl;
          return;
        }

        do
        {
          //===================================================================
          // Find all function DIE's with a name.
          //

          // We are only interested in DIE that represents a function.
          if (dwarf_tag(&child_die) == DW_TAG_subprogram)
          {
            Dwarf_Attribute decl_attr;

            // Declarations are DW_TAG_subprogram too; skip all declarations; we are only interested in definitions.
            bool is_declaration = 0;
            if (dwarf_attr(&child_die, DW_AT_declaration, &decl_attr) &&
                dwarf_formflag(&decl_attr, &is_declaration) == 0 &&
                is_declaration)
              continue;

            // Check if this is the function that contains rel_ip (pc = program counter - same thing as ip).
            // Not sure if it can happen that such a function DIE has no name, but if that is the case then
            // pretend that that isn't a function.
            char const* func_name;
            if (dwarf_haspc(&child_die, rel_ip) && (func_name = dwarf_diename(&child_die)))
            {
              printf("Found DIE for function: \"%s\"\n", func_name);

              //===============================================================
              // Get the source filename and line number.
              //
              Dwarf_Line* line = dwarf_getsrc_die(cu_die_ptr, rel_ip);
              if (line)
              {
                int lineno;
                char const* srcfile = dwarf_linesrc(line, nullptr, nullptr);
                dwarf_lineno(line, &lineno);
                std::cout << "Source-file:line-no: " << srcfile << ':' << lineno << '\n';
              }
              else
                std::cerr << "Failed to get source file and line number.\n";

              // This function was found; we're done.
              found_die = true;
              break;
            }
          }
          //===================================================================
          // Continue with the next child DIE of the compilation unit.
        }
        while (dwarf_siblingof(&child_die, &child_die) == 0);
      }
    }
    else
      std::cerr << "dwarf_offdie failed: " << dwarf_errmsg(-1) << std::endl;

    // Are we done?
    if (found_die)
      break;

    //=========================================================================
    // Next compilation unit.
    //
    // Update the offset for the next iteration.
    offset = next_offset;
  }

  // Clean up.
  dwarf_end(dwarf);
  close(fd);
}

void print_stacktrace()
{
  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize libunwind.
  unw_getcontext(&context);                     // This is the first line that is resolved when doing --ip (see below).
  unw_init_local(&cursor, &context);

  // Walk the stack frames.
  while (unw_step(&cursor) > 0)                 // Skip the above line and start with the caller.
  {
    // Get instruction pointer.
    unw_word_t ip;
    unw_get_reg(&cursor, UNW_REG_IP, &ip);

    std::cout << "\nip = " << (void*)ip << std::endl;

    if (ip == 0)
      break;

    --ip;

    // Get function name and offset.
    char sym[256];
    unw_word_t offset;
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
    {
      std::cout << "sym = \"" << sym << "\"; offset = " << offset << std::endl;

      resolve_address((void*)ip);
    }
    else
      std::cerr << "unw_get_proc_name failed!" << std::endl;
  }
}

int main()
{
  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize libunwind.
  unw_getcontext(&context);                     // This is the first line that is resolved when doing --ip (see below).
  unw_init_local(&cursor, &context);

  unw_word_t ip;
  unw_get_reg(&cursor, UNW_REG_IP, &ip);
  --ip;

  resolve_address((void*)ip);

#ifdef WRITE_PROC_MAPS
  writeProcMapsToFile();
#endif

  std::cout << "Calling print_stacktrace()\n";
  print_stacktrace();
}
