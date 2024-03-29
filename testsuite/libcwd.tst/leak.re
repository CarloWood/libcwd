// input lines 3
// output till ^NOTICE
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
NOTICE  : This output causes memory to be allocated\.
MALLOC  : malloc\(1111\) = 0x[0-9a-f]* \[leak\.cc:89\]
MALLOC  : malloc\(2222\) = 0x[0-9a-f]* \[leak\.cc:94\]
MALLOC  : malloc\(3333\) = 0x[0-9a-f]* \[leak\.cc:99\]
MALLOC  : malloc\(4444\) = 0x[0-9a-f]* \[leak\.cc:104\]
MALLOC  : operator new \(size = (8|16)\) = 0x[0-9a-f]* \[leak\.cc:113\]
// input lines 2
// output till ^MALLOC
(WARNING :     Object file (/usr/lib/debug)*/lib/libc-2.[.0-9]*.so does not have debug info.*
)*
MALLOC  : New libcwd::marker_ct at 0x[0-9a-f]*
MALLOC  : operator new \(size = (16|32)\) = 0x[0-9a-f]* \[leak\.cc:116\]
MALLOC  : operator new \(size = (8|16)\) = 0x[0-9a-f]* \[leak.cc:53\]
MALLOC  : malloc\(1212\) = 0x[0-9a-f]* \[leak\.cc:34\]
MALLOC  : malloc\(7334\) = 0x[0-9a-f]* \[leak\.cc:36\]
MALLOC  : malloc\(12345\) = 0x[0-9a-f]* \[leak\.cc:55\]
MALLOC  : malloc\(111\) = 0x[0-9a-f]* \[leak\.cc:57\]
MALLOC  : operator new \(size = (8|16)\) = 0x[0-9a-f]* \[leak\.cc:59\]
MALLOC  : malloc\(1212\) = 0x[0-9a-f]* \[leak\.cc:34\]
MALLOC  : malloc\(7334\) = 0x[0-9a-f]* \[leak\.cc:36\]
MALLOC  : malloc\(5555\) = 0x[0-9a-f]* \[leak\.cc:123\]
MALLOC  : operator new \(size = (8|16)\) = 0x[0-9a-f]* \[leak\.cc:53\]
MALLOC  : malloc\(1212\) = 0x[0-9a-f]* \[leak\.cc:34\]
MALLOC  : malloc\(7334\) = 0x[0-9a-f]* \[leak\.cc:36\]
MALLOC  : malloc\(12345\) = 0x[0-9a-f]* \[leak\.cc:55\]
MALLOC  : malloc\(111\) = 0x[0-9a-f]* \[leak\.cc:57\]
MALLOC  : operator new \(size = (8|16)\) = 0x[0-9a-f]* \[leak\.cc:59\]
MALLOC  : malloc\(1212\) = 0x[0-9a-f]* \[leak.cc:34\]
MALLOC  : malloc\(7334\) = 0x[0-9a-f]* \[leak\.cc:36\]
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (75817|75873) bytes in 23 blocks:
\(MARKER\)  0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:59   AA; \(sz = (8|16)\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:53   AA; \(sz = (8|16)\)  test_object::a
    malloc    0x[0-9a-f]*              leak\.cc:123  void\*; \(sz = 5555\)  ptr5
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:59   AA; \(sz = (8|16)\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:53   AA; \(sz = (8|16)\)  test_object::a
              0x[0-9a-f]*              leak\.cc:116  test_object; \(sz = (16|32)\) 
malloc    0x[0-9a-f]*              leak\.cc:104  void\*; \(sz = 4444\)  ptr4
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
malloc    0x[0-9a-f]*              leak\.cc:89   void\*; \(sz = 1111\)  ptr1
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:59   AA; \(sz = (8|16)\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:53   AA; \(sz = (8|16)\)  test_object::a 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:89   void\*; \(sz = 1111\)  ptr1 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:104  void\*; \(sz = 4444\)  ptr4 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:59   AA; \(sz = (8|16)\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:53   AA; \(sz = (8|16)\)  test_object::a 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:116  test_object; \(sz = (16|32)\)  
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:123  void\*; \(sz = 5555\)  ptr5 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (35121|35129) bytes in 9 blocks:
\(MARKER\)  0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : Removing libcwd::marker_ct at 0x[0-9a-f]* \(test marker\)
  \* WARNING : Memory leak detected!
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (35010|35018) bytes in 8 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (27676|27684) bytes in 7 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (20342|20350) bytes in 6 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (13008|13016) bytes in 5 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: (12897|12905) bytes in 4 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = (8|16)\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: 5555 bytes in 2 blocks:
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:94   void\*; \(sz = 2222\)  ptr2 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:99   void\*; \(sz = 3333\)  ptr3 
