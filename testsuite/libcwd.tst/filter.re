// type regexp
MALLOC  : Allocated memory: [0-9]* bytes in [2|3] blocks:
          0x[0-9a-f]* tst_filter_shared:libcwd\.tst/filter\.cc:49   std::vector<int, std::allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : Allocated memory: [0-9]* bytes in [2|3] blocks:
          0x[0-9a-f]* tst_filter_shared:libcwd\.tst/filter\.cc:49   std::vector<int, std::allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : delete 0x[0-9a-f]* <pre libcwd initialization> <unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x[0-9a-f]*            filter\.cc:49   std::vector<int, std::allocator<int> >; \(sz = 12\)  filter\.cc 
MALLOC  : malloc\(12\) = 0x[0-9a-f]*
MALLOC  : calloc\(544, 1\) = 0x[0-9a-f]*
MALLOC  : malloc\(140\) = 0x[0-9a-f]*
MALLOC  : malloc\(52\) = 0x[0-9a-f]*
MALLOC  : calloc\(5, 16\) = 0x[0-9a-f]*
MALLOC  : malloc\(68\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 4\) = 0x[0-9a-f]*
MALLOC  : New libcw::debug::marker_ct at 0x[0-9a-f]*
MALLOC  : malloc\(500\) = 0x[0-9a-f]*
MALLOC  : malloc\(123\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 4\) = 0x[0-9a-f]*
MALLOC  : New libcw::debug::marker_ct at 0x[0-9a-f]*
MALLOC  : realloc\(0x[0-9a-f]*, 1000\) = 0x[0-9a-f]*
MALLOC  : malloc\(600\) = 0x[0-9a-f]*
MALLOC  : realloc\(0x[0-9a-f]*, 1000\) = 0x[0-9a-f]*
MALLOC  : operator new\[\] \(size = 1000\) = 0x[0-9a-f]*
MALLOC  : Allocated memory: [0-9]* bytes in 1[2|3] blocks:
[0-9:.]* \(MARKER\)  0x[0-9a-f]* tst_filter_shared:           filter\.cc:126  <marker>; \(sz = 4\)  marker1
    [0-9:.]* \(MARKER\)  0x[0-9a-f]* tst_filter_shared:           filter\.cc:130  <marker>; \(sz = 4\)  marker2
        [0-9:.]* new\[\]     0x[0-9a-f]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x[0-9a-f]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[0-9a-f]* tst_filter_shared:           filter\.cc:128  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Removing libcw::debug::marker_ct at 0x[0-9a-f]* \(marker2\)
MALLOC  : delete 0x[0-9a-f]*            filter\.cc:130  <marker>; \(sz = 4\)  marker2 
MALLOC  : Removing libcw::debug::marker_ct at 0x[0-9a-f]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x[0-9a-f]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x[0-9a-f]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
  \* malloc    0x[0-9a-f]* tst_filter_shared:libcwd\.tst/filter\.cc:128  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x[0-9a-f]*            filter\.cc:126  <marker>; \(sz = 4\)  marker1 
MALLOC  : Allocated memory: [0-9]* bytes in 1[1|2] blocks:
[0-9:.]* \(deleted\) 0x[0-9a-f]* tst_filter_shared:           filter\.cc:126  <marker>; \(sz = 4\)  marker1
    [0-9:.]* realloc   0x[0-9a-f]* module\.so:           module\.cc:31   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x[0-9a-f]* module\.so:           module\.cc:38   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* malloc    0x[0-9a-f]* tst_filter_shared:           filter\.cc:128  void\*; \(sz = 123\)  Allocated between the two markers
