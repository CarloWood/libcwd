// input lines 2
// output till ^MALLOC
(BFD     : Loading debug info from.*
)*
// input lines 4
// output till ^BFD     : Loading
(MALLOC  : (malloc\([0-9]*\)|calloc\([0-9]*, [0-9]*\)) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to dl-[a-z]*\.c:[0-9]*
MALLOC  : <continued> 0x[0-9a-f]*
)+
BFD     : Loading debug info from \./module\.so \(0x[0-9a-f]*\) \.\.\. done \([0-9]* symbols\)
MALLOC  : malloc\(310\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:19
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : malloc\(300\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:10
MALLOC  : <continued> 0x[0-9a-f]*
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
malloc    0x[0-9a-f]*            module\.cc:10   void\*; \(sz = 300\)  Allocated inside static_test_symbol
malloc    0x[0-9a-f]*            module.cc:19   void\*; \(sz = 310\)  Allocated inside global_test_symbol
// input lines 2
// output till ^NOTICE
(malloc    0x[0-9a-f]* *(dl-[a-z]*\.c|stl_alloc\.h):[0-9]* *<unknown type>; \(sz = [0-9]*\) 
)+
NOTICE  : Finished
