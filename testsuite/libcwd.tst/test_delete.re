// input lines 3
// output till ^MALLOC
((WARNING : core size is limited.*
)*BFD     : Loading debug info from.*
)*
MALLOC  : operator new \(size = 1\) = 0x[0-9a-f]*
NOTICE  : Before making allocation invisible:
MALLOC  : Allocated memory: 1 bytes in 1 blocks.
          0x[0-9a-f]*       test_delete\.cc:43   A; \(sz = 1\)  Test object that we will make invisible
NOTICE  : After making allocation invisible:
MALLOC  : Allocated memory: 0 bytes in 0 blocks\.
NOTICE  : Finished successfully\.
