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
          0x5[56][0-9a-f]* tst_filter_shared:.*/testsuite/libcwd\.tst/filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : Allocated memory: 8216 bytes in 3 blocks\.
          0x5[56][0-9a-f]* tst_filter_shared:.*/testsuite/libcwd\.tst/filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : delete 0x5[56][0-9a-f]*      new_allocator\.h:115  <unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x5[56][0-9a-f]*            filter\.cc:54   std::vector<int, std::allocator<int> >; \(sz = 24\)  filter\.cc 
// input lines 3
// output till ^MALLOC  : (Trying|calloc)
MALLOC.*
WARNING.*
MALLOC.*
// input lines 2
// output till ^MALLOC  : calloc
(MALLOC  : Trying to free NULL - ignored\.
)*
MALLOC  : calloc\(1204, 1\) = 0x5[56][0-9a-f]* \[(dl-object\.c:73|ld-linux-x86-64\.so\.2:_dl_new_object)\]
MALLOC  : realloc\(0x0, 140\) = 0x5[56][0-9a-f]* \[(dl-object\.c:182|ld-linux-x86-64\.so\.2:_dl_new_object)\]
MALLOC  : malloc\(104\) = 0x5[56][0-9a-f]* \[(dl-deps\.c:474|ld-linux-x86-64\.so\.2:_dl_map_object_deps)\]
MALLOC  : calloc\(8, 24\) = 0x5[56][0-9a-f]* \[(dl-version\.c:274|ld-linux-x86-64\.so\.2:_dl_check_map_versions)\]
MALLOC  : malloc\(136\) = 0x5[56][0-9a-f]* \[(dl-open\.c:105|ld-linux-x86-64\.so\.2:add_to_global_resize)\]
// input lines 2
// output till ^(NOTICE  : dlopen|WARNING)
(MALLOC  : Trying to free NULL - ignored\.
)*
// input lines 2
// output till ^NOTICE  : dlopen
(WARNING : Cannot find symbol _end in file \./module\.so
)*
NOTICE  : dlopen\(\./module\.so, RTLD_NOW\|RTLD_GLOBAL\) == 0x5[56][0-9a-f]*
MALLOC  : operator new \(size = 16\) = 0x5[56][0-9a-f]* \[filter\.cc:191\]
MALLOC  : New libcwd::marker_ct at 0x5[56][0-9a-f]*
MALLOC  : malloc\(500\) = 0x5[56][0-9a-f]* \[filter\.cc:193\]
MALLOC  : malloc\(123\) = 0x5[56][0-9a-f]* \[filter\.cc:194\]
MALLOC  : operator new \(size = 16\) = 0x5[56][0-9a-f]* \[filter\.cc:197\]
MALLOC  : New libcwd::marker_ct at 0x5[56][0-9a-f]*
MALLOC  : realloc\(0x5[56][0-9a-f]*, 1000\) = 0x5[56][0-9a-f]* \[module\.cc:34\]
MALLOC  : malloc\(600\) = 0x5[56][0-9a-f]* \[filter\.cc:200\]
MALLOC  : realloc\(0x5[56][0-9a-f]*, 1000\) = 0x5[56][0-9a-f]* \[module\.cc:39\]
MALLOC  : operator new\[\] \(size = 1000\) = 0x5[56][0-9a-f]* \[module\.cc:48\]
MALLOC  : Allocated memory: 91[0-9][0-9] bytes in 15 blocks\.
[0-9:.]* \(MARKER\)  0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* \(MARKER\)  0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:197  <marker>; \(sz = 16\)  marker2
        [0-9:.]* new\[\]     0x5[56][0-9a-f]* module\.so:           module\.cc:48   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x5[56][0-9a-f]* module\.so:           module\.cc:39   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 5\.
MALLOC  : Removing libcwd::marker_ct at 0x5[56][0-9a-f]* \(marker2\)
MALLOC  : delete 0x5[56][0-9a-f]*            filter\.cc:197  <marker>; \(sz = 16\)  marker2 
MALLOC  : Removing libcwd::marker_ct at 0x5[56][0-9a-f]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x5[56][0-9a-f]* module\.so:.*/testsuite/module\.cc:39   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x5[56][0-9a-f]* module\.so:.*/testsuite/module\.cc:48   char\[1000\]; \(sz = 1000\)  new1000
  \* malloc    0x5[56][0-9a-f]* tst_filter_shared:.*/testsuite/libcwd\.tst/filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x5[56][0-9a-f]*            filter\.cc:191  <marker>; \(sz = 16\)  marker1 
MALLOC  : Allocated memory: 9[01][0-9][0-9] bytes in 14 blocks\.
\(deleted\) 0x5[56][0-9a-f]* main           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    realloc   0x5[56][0-9a-f]* _Z25realloc1000_with_AllocTagPv           module\.cc:39   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    new\[\]     0x5[56][0-9a-f]* _Z7new1000m           module\.cc:48   char\[1000\]; \(sz = 1000\)  new1000
    malloc    0x5[56][0-9a-f]* main           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 4\.
MALLOC  : Allocated memory: 9[01][0-9][0-9] bytes in 14 blocks\.
[0-9:.]* \(deleted\) 0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* realloc   0x5[56][0-9a-f]* module\.so:           module\.cc:39   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x5[56][0-9a-f]* module\.so:           module\.cc:48   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* malloc    0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 4\.
MALLOC  : delete\[\] 0x5[56][0-9a-f]*            module\.cc:48   char\[1000\]; \(sz = 1000\)  new1000 
NOTICE  : dlclose\(0x5[56][0-9a-f]*\)
// input lines 2
// output till ^MALLOC  : Allocated
(MALLOC  : free\(0x5[56][0-9a-f]*\) _dl_.*
)*
MALLOC  : Allocated memory: 6435 bytes in 8 blocks\.
[0-9:.]* \(deleted\) 0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:191  <marker>; \(sz = 16\)  marker1
    [0-9:.]* realloc   0x5[56][0-9a-f]* module\.so:           module\.cc:39   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x5[56][0-9a-f]* tst_filter_shared:           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 3\.
MALLOC  : free\(0x5[56][0-9a-f]*\)            module\.cc:39   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag 
MALLOC  : free\(0x5[56][0-9a-f]*\)            module\.cc:34   <unknown type>; \(sz = 1000\)  
MALLOC  : free\(0x5[56][0-9a-f]*\)            filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers 
