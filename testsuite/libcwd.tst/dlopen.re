// input lines 2
// output till Loading debug symbols from .*module\.so
(.*
)*
// input lines 4
// output till Adding ".*module\.so"
(BFD     : Loading debug symbols from .*module\.so \((<unfinished>
WARNING :     Object file (/usr/lib/debug)*/lib/libc-2.[.0-9]*.so does not have debug info.*
BFD     : <continued> )*0x[a-f0-9]*\) \.\.\. <unfinished>
)
BFD     :     Adding ".*module\.so", load address 0x[a-f0-9]*000, start 0x[a-f0-9]* and end 0x[a-f0-9]*
BFD     : <continued> done \([0-9]* symbols\)
// input lines 2
// output till malloc\(310\)
(.*
)*
MALLOC  : malloc\(310\) = <unfinished>
BFD     :     Loading debug info from .*module\.so\.\.\. done
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:24
MALLOC  : <continued> 0x[0-9a-f]* \[module\.cc:24\]
MALLOC  : malloc\(300\) = <unfinished>
BFD     :     address 0x[0-9a-f]* corresponds to module.cc:13
MALLOC  : <continued> 0x[0-9a-f]* \[module\.cc:13\]
MALLOC  : Allocated memory: [0-9]* bytes in [0-9]* blocks\.
malloc    0x[0-9a-f]* module.so:           module\.cc:13   void\*; \(sz = 300\)  Allocated inside static_test_symbol
malloc    0x[0-9a-f]* module.so:           module\.cc:24   void\*; \(sz = 310\)  Allocated inside global_test_symbol
// input lines 7
// output till ^MALLOC
((BFD     : Warning: Address 0x[0-9a-f]* in section \.text of object file "libstdc.*"
          does not have a line number, perhaps the unit containing the function
          `streambuf::streambuf\(int\)' wasn't compiled with flag -(g|ggdb)\?
          0x[0-9a-f]* *streambuf::streambuf\(int\) <unknown type>; \(sz = [0-9]*\) 
)|(BFD     : address 0x[0-9a-f]* corresponds to (streambuf\.cc:211|ios\.cc:326|memory:183)
          0x[0-9a-f]* *(streambuf.cc:211|ios.cc:326|memory:183) *<unknown type>; \(sz = [0-9]*\) 
))*
MALLOC  : Number of visible memory blocks: 2\.
MALLOC  : free\(0x[0-9a-f]*\) *module.cc:24 *void\*; \(sz = 310\)  Allocated inside global_test_symbol 
MALLOC  : free\(0x[0-9a-f]*\) *module.cc:13 *void\*; \(sz = 300\)  Allocated inside static_test_symbol 
// input lines 2
// output till ^(MALLOC|NOTICE)
(WARNING : This compiler version is buggy, a call to dlclose\(\) will destruct the standard streams.*
)*
// input lines 2
// output till ^NOTICE
(MALLOC  : .*
)*
NOTICE  : Finished
