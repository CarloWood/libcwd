// input lines 2
// output till ^MALLOC
(STABS   : Loading debug info from.*
)*
MALLOC  : malloc\(1111\) = 0x[0-9a-f]*
MALLOC  : malloc\(2222\) = 0x[0-9a-f]*
MALLOC  : malloc\(3333\) = 0x[0-9a-f]*
MALLOC  : malloc\(4444\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 1\) = 0x[0-9a-f]*
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
MALLOC  : Allocated memory: 75810 bytes in 23 blocks\.
\(MARKER\)  0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:55   AA; \(sz = 8\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:51   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:49   AA; \(sz = 8\)  test_object::a
    malloc    0x[0-9a-f]*              leak\.cc:121  void\*; \(sz = 5555\)  ptr5
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:55   AA; \(sz = 8\)  test_object::b
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:51   void\*; \(sz = 12345\)  test_object::ptr
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr
              0x[0-9a-f]*              leak\.cc:49   AA; \(sz = 8\)  test_object::a
              0x[0-9a-f]*              leak\.cc:114  test_object; \(sz = 16\) 
malloc    0x[0-9a-f]*              leak\.cc:102  void\*; \(sz = 4444\)  ptr4
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
malloc    0x[0-9a-f]*              leak\.cc:87   void\*; \(sz = 1111\)  ptr1
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:55   AA; \(sz = 8\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:51   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:49   AA; \(sz = 8\)  test_object::a 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:87   void\*; \(sz = 1111\)  ptr1 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:102  void\*; \(sz = 4444\)  ptr4 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:55   AA; \(sz = 8\)  test_object::b 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:51   void\*; \(sz = 12345\)  test_object::ptr 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:32   void\*; \(sz = 1212\)  AA::ptr 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:49   AA; \(sz = 8\)  test_object::a 
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:114  test_object; \(sz = 16\)  
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:121  void\*; \(sz = 5555\)  ptr5 
MALLOC  : Allocated memory: 35114 bytes in 9 blocks\.
\(MARKER\)  0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : Removing libcw::debug::marker_ct at 0x[0-9a-f]* \(test marker\)
  \* WARNING : Memory leak detected!
  \* malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
  \* malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
  \* malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
MALLOC  : delete 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory: 35003 bytes in 8 blocks\.
\(deleted\) 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 27669 bytes in 7 blocks\.
\(deleted\) 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 20335 bytes in 6 blocks\.
\(deleted\) 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 13001 bytes in 5 blocks\.
\(deleted\) 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
    malloc    0x[0-9a-f]*              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:53   void\*; \(sz = 111\)  test_object::leak 
MALLOC  : Allocated memory: 12890 bytes in 4 blocks\.
\(deleted\) 0x[0-9a-f]*              leak\.cc:111  <marker>; \(sz = 1\)  test marker
    malloc    0x[0-9a-f]*              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:34   void\*; \(sz = 7334\)  AA::leakAA 
MALLOC  : Allocated memory: 5555 bytes in 2 blocks\.
malloc    0x[0-9a-f]*              leak\.cc:97   void\*; \(sz = 3333\)  ptr3
malloc    0x[0-9a-f]*              leak\.cc:92   void\*; \(sz = 2222\)  ptr2
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:92   void\*; \(sz = 2222\)  ptr2 
MALLOC  : free\(0x[0-9a-f]*\)              leak\.cc:97   void\*; \(sz = 3333\)  ptr3 
