// input lines 2
// output till Loading debug info from \./module\.so
(.*
)*
BFD     : Loading debug info from \./module\.so \(0x[a-f0-9]*\) \.\.\. done \([0-9]* symbols\)
// input lines 2
// output till malloc\(310\)
(.*
)*
MALLOC  : malloc\(310\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:16
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : malloc\(300\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:7
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
malloc    0x[0-9a-f]*            module\.cc:7    void\*; \(sz = 300\)  Allocated inside static_test_symbol
malloc    0x[0-9a-f]*            module\.cc:16   void\*; \(sz = 310\)  Allocated inside global_test_symbol
// input lines 2
// output till ^NOTICE
(malloc    0x[0-9a-f]* *(dl-[a-z]*\.c|stl_alloc\.h|specific\.c|dlerror\.c):[0-9]* *<unknown type>; \(sz = [0-9]*\) 
)*
NOTICE  : Finished
