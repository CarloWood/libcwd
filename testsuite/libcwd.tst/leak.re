// input lines 3
// output till ^MALLOC
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
MALLOC  : malloc\(1111\) = 0x[0-9a-f]*
MALLOC  : malloc\(2222\) = 0x[0-9a-f]*
MALLOC  : malloc\(3333\) = 0x[0-9a-f]*
MALLOC  : malloc\(4444\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : New libcw::debug::marker_ct at 0x[0-9a-f]*
MALLOC  : operator new \(size = 16\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : malloc\(1212\) = 0x[0-9a-f]*
MALLOC  : malloc\(7334\) = 0x[0-9a-f]*
MALLOC  : malloc\(12345\) = 0x[0-9a-f]*
MALLOC  : malloc\(111\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : malloc\(1212\) = 0x[0-9a-f]*
MALLOC  : malloc\(7334\) = 0x[0-9a-f]*
MALLOC  : malloc\(5555\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : malloc\(1212\) = 0x[0-9a-f]*
MALLOC  : malloc\(7334\) = 0x[0-9a-f]*
MALLOC  : malloc\(12345\) = 0x[0-9a-f]*
MALLOC  : malloc\(111\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : malloc\(1212\) = 0x[0-9a-f]*
MALLOC  : malloc\(7334\) = 0x[0-9a-f]*
MALLOC  : Allocated memory: 75817 bytes in 23 blocks:
\(MARKER\)  0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:59   AA; \(sz = 8\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:53   AA; \(sz = 8\)  test_object::a
    malloc    0x[0-9a-f]*              leak\.cc:123  void\*; \(sz = 5555\)  ptr5
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:59   AA; \(sz = 8\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:53   AA; \(sz = 8\)  test_object::a
              0x[0-9a-f]*              leak\.cc:116  test_object; \(sz = 16\) 
malloc    0x[0-9a-f]*              leak\.cc:104  void\*; \(sz = 4444\)  ptr4
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
malloc    0x[0-9a-f]*              leak\.cc:89   void\*; \(sz = 1111\)  ptr1
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:59   AA; \(sz = 8\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:53   AA; \(sz = 8\)  test_object::a 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:89   void\*; \(sz = 1111\)  ptr1 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:104  void\*; \(sz = 4444\)  ptr4 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:59   AA; \(sz = 8\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:55   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:53   AA; \(sz = 8\)  test_object::a 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:116  test_object; \(sz = 16\)  
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:123  void\*; \(sz = 5555\)  ptr5 
MALLOC  : Allocated memory: 35121 bytes in 9 blocks:
\(MARKER\)  0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : Removing libcw::debug::marker_ct at 0x[0-9a-f]* \(test marker\)
  \* WARNING : Memory leak detected!
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory: 35010 bytes in 8 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 27676 bytes in 7 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 20342 bytes in 6 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 13008 bytes in 5 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:57   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory: 12897 bytes in 4 blocks:
\(deleted\) 0x[0-9a-f]*              leak\.cc:113  <marker>; \(sz = 8\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:36   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 5555 bytes in 2 blocks:
malloc    0x[0-9a-f]*              leak\.cc:99   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:94   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:94   void\*; \(sz = 2222\)  ptr2 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:99   void\*; \(sz = 3333\)  ptr3 
