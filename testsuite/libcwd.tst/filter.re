// type regexp
MALLOC  : operator new \(size = 12\) = 0x[0-9a-f]*
MALLOC  : malloc\(2704\) = 0x[0-9a-f]*
MALLOC  : malloc\(256\) = 0x[0-9a-f]*
MALLOC  : malloc\(512\) = 0x[0-9a-f]*
MALLOC  : free\(0x[0-9a-f]*\) .* <unknown type>; \(sz = 256\)  
MALLOC  : malloc\(1024\) = 0x[0-9a-f]*
MALLOC  : free\(0x[0-9a-f]*\) .* <unknown type>; \(sz = 512\)  
MALLOC  : malloc\(2048\) = 0x[0-9a-f]*
MALLOC  : free\(0x[0-9a-f]*\) .* <unknown type>; \(sz = 1024\)  
MALLOC  : malloc\(4096\) = 0x[0-9a-f]*
MALLOC  : free\(0x[0-9a-f]*\) .* <unknown type>; \(sz = 2048\)  
MALLOC  : Allocated memory: 6812 bytes in 3 blocks:
          0x[0-9a-f]* tst_filter_shared:/home/carlo/c../libcwd/testsuite/\./libcwd\.tst/filter\.cc:44   vector<int, allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : Allocated memory: 6812 bytes in 3 blocks:
malloc    0x[0-9a-f]* tst_filter_shared:/usr/include/g..-3/stl_alloc\.h:157  <unknown type>; \(sz = 4096\) 
malloc    0x[0-9a-f]* tst_filter_shared:/usr/include/g..-3/stl_alloc\.h:490  <unknown type>; \(sz = 2704\) 
          0x[0-9a-f]* tst_filter_shared:/home/carlo/c../libcwd/testsuite/\./libcwd\.tst/filter\.cc:44   vector<int, allocator<int> >; \(sz = 12\)  filter\.cc
MALLOC  : free\(0x[0-9a-f]*\)          stl_alloc\.h:157  <unknown type>; \(sz = 4096\)  
MALLOC  : delete 0x[0-9a-f]*            filter\.cc:44   vector<int, allocator<int> >; \(sz = 12\)  filter\.cc 
