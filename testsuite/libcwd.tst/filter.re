// type regexp
MALLOC  : Allocated memory: [0-9]* bytes in [2|3] blocks:
          0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:50   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : Allocated memory: [0-9]* bytes in [2|3] blocks:
          0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:50   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : (free\(|delete )0x[a-f0-9]*( <pre libcwd initialization>|\) <pre ios initialization> |\)          stl_alloc.h:157 ) <unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:50   (std::|)vector<int, (std::|)allocator<int> >; \(sz = 12\)  filter\.cc 
MALLOC  : malloc\(12\) = 0x[a-f0-9]*
MALLOC  : calloc\(544, 1\) = 0x[a-f0-9]*
MALLOC  : malloc\(140\) = 0x[a-f0-9]*
MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]*
MALLOC  : calloc\([3-6], 16\) = 0x[a-f0-9]*
MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]*
NOTICE  : dlopen\(\./module\.so, RTLD_NOW\|RTLD_GLOBAL\) == 0x[a-f0-9]*
MALLOC  : operator new \(size = 4\) = 0x[a-f0-9]*
MALLOC  : New libcw::debug::marker_ct at 0x[a-f0-9]*
MALLOC  : malloc\(500\) = 0x[a-f0-9]*
MALLOC  : malloc\(123\) = 0x[a-f0-9]*
MALLOC  : operator new \(size = 4\) = 0x[a-f0-9]*
MALLOC  : New libcw::debug::marker_ct at 0x[a-f0-9]*
MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = 0x[a-f0-9]*
MALLOC  : malloc\(600\) = 0x[a-f0-9]*
MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = 0x[a-f0-9]*
MALLOC  : operator new\[\] \(size = 1000\) = 0x[a-f0-9]*
MALLOC  : Allocated memory: [0-9]* bytes in 1[234] blocks:
[0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_shared:           filter\.cc:128  <marker>; \(sz = 4\)  marker1
    [0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_shared:           filter\.cc:132  <marker>; \(sz = 4\)  marker2
        [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:130  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Removing libcw::debug::marker_ct at 0x[a-f0-9]* \(marker2\)
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:132  <marker>; \(sz = 4\)  marker2 
MALLOC  : Removing libcw::debug::marker_ct at 0x[a-f0-9]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
  \* malloc    0x[a-f0-9]* tst_filter_shared:/.*/testsuite/libcwd\.tst/filter\.cc:130  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:128  <marker>; \(sz = 4\)  marker1 
MALLOC  : Allocated memory: [0-9]* bytes in 1[123] blocks:
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_shared:           filter\.cc:128  <marker>; \(sz = 4\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:130  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete\[\] 0x[a-f0-9]*            module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000 
NOTICE  : dlclose\(0x[a-f0-9]*\)
MALLOC  : free\(0x[a-f0-9]*\)         dl-version\.c:289  <unknown type>; \(sz = [0-9]*\)  
MALLOC  : free\(0x[a-f0-9]*\)          dl-object\.c:119  <unknown type>; \(sz = 140\)  
MALLOC  : free\(0x[a-f0-9]*\)            dl-load\.c:164  <unknown type>; \(sz = 12\)  
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : Trying to free NULL - ignored\.
MALLOC  : free\(0x[a-f0-9]*\)          dl-object\.c:43   <unknown type>; \(sz = 544\)  
MALLOC  : free\(0x[a-f0-9]*\)            dl-deps\.c:528  <unknown type>; \(sz = [0-9]*\)  
// input lines 3
// output till MALLOC  : Allocated memory
(MALLOC  : delete 0x[a-f0-9]* <pre libcwd initialization> <unknown type>; \(sz = 24\)  
MALLOC  : Trying to free NULL - ignored\.
)*
MALLOC  : Allocated memory: [0-9]* bytes in [567] blocks:
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_shared:           filter\.cc:128  <marker>; \(sz = 4\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_shared:           filter\.cc:130  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag 
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:26   <unknown type>; \(sz = 1000\)  
MALLOC  : free\(0x[a-f0-9]*\)            filter\.cc:130  void\*; \(sz = 123\)  Allocated between the two markers 
