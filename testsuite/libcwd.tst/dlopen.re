// input lines 2
// output till Loading debug info from \./module\.so
(.*
)*
BFD     : Loading debug info from \./module\.so \(0x[a-f0-9]*\) \.\.\. done \([0-9]* symbols\)
// input lines 2
// output till malloc\(310\)
(.*
)*
MALLOC  : malloc\(310\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:19
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : malloc\(300\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:10
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks:
malloc    0x[0-9a-f]*            module\.cc:10   void\*; \(sz = 300\)  Allocated inside static_test_symbol
malloc    0x[0-9a-f]*            module\.cc:19   void\*; \(sz = 310\)  Allocated inside global_test_symbol
// input lines 11
// output till ^MALLOC
((BFD     : Warning: Address 0x[0-9a-f]* in section \.text of object file "libstdc.*"
          does not have a line number, perhaps the unit containing the function
          `streambuf::streambuf\(int\)' wasn't compiled with flag -(g|ggdb)\?
          0x[0-9a-f]* *streambuf::streambuf\(int\) <unknown type>; \(sz = [0-9]*\) 
)|(BFD     : address 0x[0-9a-f]* corresponds to streambuf.cc:211
          0x[0-9a-f]* *streambuf.cc:211 *<unknown type>; \(sz = [0-9]*\) 
))*(((malloc |realloc)   0x[0-9a-f]* *((dl-[a-z]*\.c|stl_alloc\.h|specific\.c|dlerror\.c|eh_globals\.cc):[0-9]*|add_to_global|open_path|_dl_[a-z_]*) *<unknown type>; \(sz = [0-9]*\) 
)|(BFD     : Warning: Address 0x[0-9a-f]* in section \.text of object file "ld-linux.so\.2"
          does not have a line number, perhaps the unit containing the function
          `_dl_map_object_deps' wasn't compiled with flag -(g|gdb)\?
)*)*
MALLOC  : free\(0x[0-9a-f]*\) *module.cc:19 *void\*; \(sz = 310\)  Allocated inside global_test_symbol 
MALLOC  : free\(0x[0-9a-f]*\) *module.cc:10 *void\*; \(sz = 300\)  Allocated inside static_test_symbol 
MALLOC  : free\(0x[0-9a-f]*\) *(dl-version.c:[0-9]*|_dl_check_map_versions) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : free\(0x[0-9a-f]*\) *(dl-object.c:[0-9]*|_dl_new_object) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : free\(0x[0-9a-f]*\) *(dl-load.c:[0-9]*|_dl_map_object|open_path) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : free\(0x[0-9a-f]*\) *(dl-object.c:[0-9]*|_dl_new_object) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : free\(0x[0-9a-f]*\) *(dl-deps.c:[0-9]*|_dl_map_object_deps) *<unknown type>; \(sz = [0-9]*\)  
// input lines 3
// output till ^NOTICE
(MALLOC  : delete 0x[0-9a-f]* *(streambuf::streambuf\(int\)|streambuf\.cc:211) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : Trying to free NULL - ignored\.
)*
NOTICE  : Finished
