// input lines 3
// output till ^MALLOC
((WARNING : core size is limited.*
)*(BFD     : Loading debug info from.*
)*)
// type regexp
MALLOC  : Allocated memory: [0-9]* bytes in [2-5] blocks:
          0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:54   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : Allocated memory: [0-9]* bytes in [2-5] blocks:
          0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:54   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : (free\(|delete )0x[a-f0-9]*( <pre libcwd initialization>|\) <pre ios initialization> |\)          stl_alloc.h:157|          stl_alloc.h:(103|108|109)|          stl-inst.cc:104) *<unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:54   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc 
MALLOC  : malloc\(12\) = 0x[a-f0-9]*
MALLOC  : calloc\(54[4|8], 1\) = 0x[a-f0-9]*
MALLOC  : (malloc\(140\)|realloc\(0x0, 140\)) = 0x[a-f0-9]*
MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]*
MALLOC  : calloc\([3-6], 16\) = 0x[a-f0-9]*
MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]*
NOTICE  : dlopen\(\./module\.so, RTLD_NOW\|RTLD_GLOBAL\) == 0x[a-f0-9]*
MALLOC  : operator new \(size = 8\) = 0x[a-f0-9]*
MALLOC  : New libcw::debug::marker_ct at 0x[a-f0-9]*
MALLOC  : malloc\(500\) = 0x[a-f0-9]*
MALLOC  : malloc\(123\) = 0x[a-f0-9]*
MALLOC  : operator new \(size = 8\) = 0x[a-f0-9]*
MALLOC  : New libcw::debug::marker_ct at 0x[a-f0-9]*
MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = 0x[a-f0-9]*
MALLOC  : malloc\(600\) = 0x[a-f0-9]*
MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = 0x[a-f0-9]*
MALLOC  : operator new\[\] \(size = 1000\) = 0x[a-f0-9]*
MALLOC  : Allocated memory: [0-9]* bytes in 1[0-9] blocks:
[0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_shared:           filter\.cc:181  <marker>; \(sz = 8\)  marker1
    [0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_shared:           filter\.cc:187  <marker>; \(sz = 8\)  marker2
        [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:184  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Removing libcw::debug::marker_ct at 0x[a-f0-9]* \(marker2\)
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:187  <marker>; \(sz = 8\)  marker2 
MALLOC  : Removing libcw::debug::marker_ct at 0x[a-f0-9]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
  \* malloc    0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:184  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:181  <marker>; \(sz = 8\)  marker1 
MALLOC  : Allocated memory: [0-9]* bytes in 1[0-9] blocks:
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_shared:           filter\.cc:181  <marker>; \(sz = 8\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:184  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete\[\] 0x[a-f0-9]*            module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000 
NOTICE  : dlclose\(0x[a-f0-9]*\)
MALLOC  : free\(0x[a-f0-9]*\) *(dl-version\.c:289|_dl_check_map_versions) *<unknown type>; \(sz = [0-9]*\)  
MALLOC  : free\(0x[a-f0-9]*\) *(dl-object\.c:119|_dl_new_object) *<unknown type>; \(sz = 140\)  
MALLOC  : free\(0x[a-f0-9]*\) *(dl-load\.c:164|_dl_map_object) *<unknown type>; \(sz = 12\)  
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : free\(0x[a-f0-9]*\) *(dl-object\.c:43|_dl_new_object) *<unknown type>; \(sz = 54[4|8]\)  
MALLOC  : free\(0x[a-f0-9]*\) *(dl-deps\.c:528|_dl_map_object_deps|<pre libcwd initialization>) *<unknown type>; \(sz = [0-9]*\)  
// input lines 4
// output till MALLOC  : Allocated memory
((MALLOC  : .*<pre libcwd initialization> <unknown type>; \(sz = [0-9]*\)  
MALLOC  : (Trying to free NULL - ignored\.|free\(0x[a-f0-9]*\) <pre libcwd initialization> <unknown type>.*)
)*|(MALLOC  : delete 0x[a-f0-9]* <pre ios initialization>  <unknown type>.*
))
MALLOC  : Allocated memory: [0-9]* bytes in [5-9] blocks:
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_shared:           filter\.cc:181  <marker>; \(sz = 8\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:184  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag 
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:26   <unknown type>; \(sz = 1000\)  
MALLOC  : free\(0x[a-f0-9]*\)            filter\.cc:184  void\*; \(sz = 123\)  Allocated between the two markers 
