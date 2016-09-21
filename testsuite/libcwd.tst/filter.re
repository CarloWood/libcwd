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
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
          0x[a-f0-9]* tst_filter_(static|shared):/.*/testsuite/libcwd\.tst/filter\.cc:54   (std::|__gnu_norm::|__gnu_debug_def::|)vector<int, (std::|)allocator<int> >; \(sz = (12|24|28)\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
          0x[a-f0-9]* tst_filter_(static|shared):/.*/testsuite/libcwd\.tst/filter\.cc:54   (std::|__gnu_norm::|__gnu_debug_def::|)vector<int, (std::|)allocator<int> >; \(sz = (12|24|28)\)  filter\.cc
MALLOC  : Number of visible memory blocks: 1\.
MALLOC  : (free\(|delete )0x[a-f0-9]*( <pre libcwd initialization>|\) <pre ios initialization> |\)          stl_alloc.h:(115|157)|          stl_alloc.h:(103|108|109)|          stl-inst.cc:104|      new_allocator.h:([89][0-9]|104)|     pool_allocator.h:(293|294)|       mt_allocator.h:[0-9]*|                     ) *<unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:54   (std::|__gnu_norm::|__gnu_debug_def::|)vector<int, (std::|)allocator<int> >; \(sz = (12|24|28)\)  filter\.cc 
// input lines 5
// output till ^MALLOC  : calloc
(MALLOC  : malloc\(12\) = <unfinished>
WARNING :     Object file (/usr/lib/debug)*/lib/ld-2\.[.0-9]*\.so does not have debug info\.  Address lookups inside this object file will result in a function name only, not a source file location\.
MALLOC  : <continued> 0x[a-f0-9]* \[ld-linux\.so\.2:(_dl_map_object|expand_dynamic_string_token)\]
)|(MALLOC  : malloc\(12\) = 0x[a-f0-9]* \[(ld-linux\.so\.2:(_dl_map_object|expand_dynamic_string_token)|dl-load\.c:(170|162)|ld-linux-x86-64\.so\.2:<unknown function>)\]
)
// input lines 5
// output till MALLOC  : (malloc|realloc)
(MALLOC  : calloc\(1172, 1\) = <unfinished>
WARNING :     Object file (/usr/lib/debug)*/lib64/ld-2\.[.0-9]*\.so does not have debug info\.  Address lookups inside this object file will result in a function name only, not a source file location\.
MALLOC  : <continued> 0x[a-f0-9]* \[ld-linux-x86-64\.so\.2:_dl_rtld_di_serinfo\]
)|(MALLOC  : calloc\((1180|1172|1156|612|600|616|572|544|548|576|588|596), 1\) = 0x[a-f0-9]* \[(ld-linux\.so\.2:_dl_new_object|dl-object\.c:(52|77|75))\]
)
MALLOC  : (malloc\(140\)|realloc\(0x0, 140\)) = 0x[a-f0-9]* \[(ld-linux-x86-64\.so\.2:_dl_rtld_di_serinfo|ld-linux\.so\.2:_dl_new_object|dl-object\.c:(160|161|180|184))\]
MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]* \[(ld-linux-x86-64\.so\.2:_dl_rtld_di_serinfo|ld-linux\.so\.2:_dl_map_object_deps|dl-deps\.c:(50[067]|511))\]
MALLOC  : calloc\([3-8], (16|24)\) = 0x[a-f0-9]* \[(ld-linux-x86-64\.so\.2:_dl_debug_state|ld-linux\.so\.2:_dl_check_map_versions|dl-version\.c:(299|293))\]
// input lines 5
// output till ^NOTICE  : dlopen
(MALLOC  : malloc\([0-9]*\) = <unfinished>
WARNING :     Object file .*/libc-2.[.0-9]*.so does not have debug info\.  Address lookups inside this object file will result in a function name only, not a source file location\.
MALLOC  : <continued> 0x[a-f0-9]* \[libc\.so\.6:(dl_open_worker|_dl_open|add_to_global)\]
)|(MALLOC  : malloc\([0-9]*\) = 0x[a-f0-9]* \[(libc\.so\.6:(dl_open_worker|_dl_open)|ld-linux-x86-64\.so\.2:_dl_make_stack_executable|ld-linux.so\.2:add_to_global|dl-open\.c:10[4678])\]
)
NOTICE  : dlopen\(\./module\.so, RTLD_NOW\|RTLD_GLOBAL\) == 0x[a-f0-9]*
MALLOC  : operator new \(size = (8|16)\) = 0x[a-f0-9]* \[filter\.cc:191\]
MALLOC  : New libcwd::marker_ct at 0x[a-f0-9]*
MALLOC  : malloc\(500\) = 0x[a-f0-9]* \[filter\.cc:193\]
MALLOC  : malloc\(123\) = 0x[a-f0-9]* \[filter\.cc:194\]
MALLOC  : operator new \(size = (8|16)\) = 0x[a-f0-9]* \[filter\.cc:197\]
MALLOC  : New libcwd::marker_ct at 0x[a-f0-9]*
// input lines 4
// output till ^MALLOC  : malloc\(600\)
(MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = (|<unfinished>
BFD     :     Loading debug info from .*/module\.so\.\.\. done
MALLOC  : <continued> )0x[a-f0-9]* \[module\.cc:33\]
)
MALLOC  : malloc\(600\) = 0x[a-f0-9]* \[filter\.cc:200\]
MALLOC  : realloc\(0x[a-f0-9]*, 1000\) = 0x[a-f0-9]* \[module\.cc:38\]
MALLOC  : operator new\[\] \(size = 1000\) = 0x[a-f0-9]* \[module\.cc:47\]
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
// input lines 2
// output till ^[0-9:.]* \(MARKER\)
(WARNING : Object file /lib/libc\.so.* does not have debug info.*
)*
[0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:191  <marker>; \(sz = (8|16)\)  marker1
    [0-9:.]* \(MARKER\)  0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:197  <marker>; \(sz = (8|16)\)  marker2
        [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
        [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 5\.
MALLOC  : Removing libcwd::marker_ct at 0x[a-f0-9]* \(marker2\)
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:197  <marker>; \(sz = (8|16)\)  marker2 
MALLOC  : Removing libcwd::marker_ct at 0x[a-f0-9]* \(marker1\)
  \* WARNING : Memory leak detected!
  \* realloc   0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
  \* new\[\]     0x[a-f0-9]* module\.so:/.*/testsuite/module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
  \* malloc    0x[a-f0-9]* tst_filter_(static|shared):/.*/testsuite/libcwd\.tst/filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : delete 0x[a-f0-9]*            filter\.cc:191  <marker>; \(sz = (8|16)\)  marker1 
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
\(deleted\) 0x[a-f0-9]* main           filter\.cc:191  <marker>; \(sz = (8|16)\)  marker1
    realloc   0x[a-f0-9]* _Z25realloc1000_with_AllocTagPv           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    new\[\]     0x[a-f0-9]* _Z7new1000(j|m)           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
    malloc    0x[a-f0-9]* main           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 4\.
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:191  <marker>; \(sz = (8|16)\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:38   void\*; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* new\[\]     0x[a-f0-9]* module\.so:           module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 4\.
MALLOC  : delete\[\] 0x[a-f0-9]*            module\.cc:47   char\[1000\]; \(sz = 1000\)  new1000 
NOTICE  : dlclose\(0x[a-f0-9]*\)
// input lines 2
// output till ^MALLOC
(WARNING : This compiler version is buggy, a call to dlclose\(\) will destruct the standard streams.*
)*
// input lines 3
// output till MALLOC  : Allocated memory
((MALLOC  : Trying to free NULL - ignored\.
)|(MALLOC  : free\(0x[a-f0-9]*\) (_dl_[a-z_]*|) *<unknown type>; \(sz = [0-9]+\)  
))*
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
// input lines 2
// output till tst_filter_
([0-9:.]* *0x[a-f0-9]* libcwd\.so\.0:              bfd\.cc:1684 std::ios_base::Init; \(sz = 1\)  Bug workaround\.  See WARNING about dlclose\(\) above\.
)*
[0-9:.]* \(deleted\) 0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:191  <marker>; \(sz = (8|16)\)  marker1
    [0-9:.]* realloc   0x[a-f0-9]* module\.so:           module\.cc:38   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag
    [0-9:.]* malloc    0x[a-f0-9]* tst_filter_(static|shared):           filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers
MALLOC  : Number of visible memory blocks: 3\.
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:38   <unknown type>; \(sz = 1000\)  realloc1000_with_AllocTag 
MALLOC  : free\(0x[a-f0-9]*\)            module\.cc:33   <unknown type>; \(sz = 1000\)  
MALLOC  : free\(0x[a-f0-9]*\)            filter\.cc:194  void\*; \(sz = 123\)  Allocated between the two markers 
