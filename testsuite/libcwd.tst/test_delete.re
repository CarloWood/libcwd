// input lines 3
// output till ^NOTICE
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
NOTICE  : This write causes memory to be allocated\.
MALLOC  : operator new \(size = 1\) = 0x[0-9a-f]* \[test_delete\.cc:44\]
NOTICE  : Before making allocation invisible:
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: 1 bytes in 1 blocks\.
          0x[0-9a-f]*       test_delete\.cc:44   At; \(sz = 1\)  Test object that we will make invisible
NOTICE  : After making allocation invisible:
MALLOC  : Allocated memory by thread 0x[0-9a-f]*: 0 bytes in 0 blocks\.
NOTICE  : Finished successfully\.
