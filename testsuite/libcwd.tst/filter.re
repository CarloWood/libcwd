// input lines 4
// output till ^(MALLOC|WARNING :  )
(WARNING : core size is limited.*
)*((WARNING : Object file .* does not have debug info\..*
)*(BFD     : Loading debug.*
)*)*
// input lines 2
// output till ^MALLOC
(WARNING :     Object file (/usr/lib/debug)*/lib/libc-2.[.0-9]*.so does not have debug info.*
)*
// type regexp
MALLOC  : Allocated memory: 4152 bytes in 3 blocks\.
          0x5[56][0-9a-f]* tst_filter_shared:/home/carlo/projects/libcwd/libcwd/testsuite/libcwd\.tst/filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : Allocated memory: 8216 bytes in 3 blocks\.
          0x5[56][0-9a-f]* tst_filter_shared:/home/carlo/projects/libcwd/libcwd/testsuite/libcwd\.tst/filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : delete 0x55[0-9a-f]*      new_allocator\.h:111  <unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x55[0-9a-f]*            filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc 
MALLOC  : malloc\(12\) = 0x55[0-9a-f]* \[strdup\.c:42\]
MALLOC  : calloc\(1180, 1\) = 0x55[0-9a-f]* \[dl-object\.c:73\]
MALLOC  : realloc\(0x0, 140\) = 0x55[0-9a-f]* \[dl-object\.c:182\]
MALLOC  : malloc\(104\) = 0x55[0-9a-f]* \[dl-deps\.c:474\]
MALLOC  : calloc\(10, 24\) = 0x55[0-9a-f]* \[dl-version\.c:274\]
MALLOC  : malloc\(136\) = 0x55[0-9a-f]* \[dl-open\.c:105\]
NOTICE  : dlopen\(\./module\.so, RTLD_NOW\|RTLD_GLOBAL\) == 0x55[0-9a-f]*
MALLOC  : operator new \(size = 16\) = 0x55[0-9a-f]* \[filter\.cc:191\]
MALLOC  : New libcwd::marker_ct at 0x55[0-9a-f]*
MALLOC  : malloc\(500\) = 0x55[0-9a-f]* \[filter\.cc:193\]
MALLOC  : malloc\(123\) = 0x55[0-9a-f]* \[filter\.cc:194\]
MALLOC  : operator new \(size = 16\) = 0x55[0-9a-f]* \[filter\.cc:197\]
MALLOC  : New libcwd::marker_ct at 0x55[0-9a-f]*
MALLOC  : realloc\(0x55[0-9a-f]*, 1000\) = 0x55[0-9a-f]* \[filter\.cc:199\]
MALLOC  : malloc\(600\) = 0x55[0-9a-f]* \[filter\.cc:200\]
MALLOC  : realloc\(0x55[0-9a-f]*, 1000\) = 0x55[0-9a-f]* \[module\.cc:38\]
MALLOC  : operator new\[\] \(size = 1000\) = 0x55[0-9a-f]* \[module\.cc:47\]
MALLOC  : Allocated memory: 9127 bytes in 15 blocks\.
[0-9:.]* \(MARKER\)  0x55[0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* \(MARKER\)  0x55[0-9a-f]* tst_filter_shared:           filter\.cc:197  <marker>; \(sz = 16\)  marker2
        [0-9:.]* new\[\]     0x55[0-9a-f]* module\.so:           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x55[0-9a-f]* module\.so:           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x55[0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 5\.
MALLOC  : Removing libcwd::marker_ct at 0x55[0-9a-f]* \(marker2\)
  \* WARNING : Memory leak detected!
  \* realloc   0x55[0-9a-f]* tst_filter_shared:/home/carlo/projects/libcwd/libcwd/testsuite/libcwd\.tst/filter\.cc:199  <unknown type>; \(sz = 1000\) 
MALLOC  : delete 0x55[0-9a-f]*            filter\.cc:197  <marker>; \(sz = 16\)  marker2 
MALLOC  : Removing libcwd::marker_ct at 0x55[0-9a-f]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x55[0-9a-f]* module\.so:/home/carlo/projects/libcwd/libcwd/testsuite/module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x55[0-9a-f]* module\.so:/home/carlo/projects/libcwd/libcwd/testsuite/module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
  \* \(deleted\) 0x55[0-9a-f]* tst_filter_shared:/home/carlo/projects/libcwd/libcwd/testsuite/libcwd\.tst/filter\.cc:197  <marker>; \(sz = 16\)  marker2
  \* malloc    0x55[0-9a-f]* tst_filter_shared:/home/carlo/projects/libcwd/libcwd/testsuite/libcwd\.tst/filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x55[0-9a-f]*            filter\.cc:191  <marker>; \(sz = 16\)  marker1 
MALLOC  : Allocated memory: 9127 bytes in 15 blocks\.
\(deleted\) 0x55[0-9a-f]* main           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    realloc   0x55[0-9a-f]* _Z25realloc1000_with_AllocTagPv           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    new\[\]     0x55[0-9a-f]* _Z7new1000m           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
    \(deleted\) 0x55[0-9a-f]* main           filter\.cc:197  <marker>; \(sz = 16\)  marker2
    malloc    0x55[0-9a-f]* main           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 5\.
MALLOC  : Allocated memory: 9127 bytes in 15 blocks\.
[0-9:.]* \(deleted\) 0x55[0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* realloc   0x55[0-9a-f]* module\.so:           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x55[0-9a-f]* module\.so:           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* \(deleted\) 0x55[0-9a-f]* tst_filter_shared:           filter\.cc:197  <marker>; \(sz = 16\)  marker2
    [0-9:.]* malloc    0x55[0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 5\.
MALLOC  : delete\[\] 0x55[0-9a-f]*            module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000 
NOTICE  : dlclose\(0x55[0-9a-f]*\)
MALLOC  : Allocated memory: 8127 bytes in 14 blocks\.
[0-9:.]* \(deleted\) 0x55[0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* realloc   0x55[0-9a-f]* module\.so:           module\.cc:38   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* \(deleted\) 0x55[0-9a-f]* tst_filter_shared:           filter\.cc:197  <marker>; \(sz = 16\)  marker2
    [0-9:.]* malloc    0x55[0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 4\.
MALLOC  : free\(0x55[0-9a-f]*\)            module\.cc:38   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag 
MALLOC  : free\(0x55[0-9a-f]*\)            filter\.cc:199  <unknown type>; \(sz = 1000\)  
MALLOC  : free\(0x55[0-9a-f]*\)            filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers 
