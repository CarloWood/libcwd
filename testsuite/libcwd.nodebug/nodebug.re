MALLOC  : operator new\[\] \(size = 400\) = <unfinished>
// input lines 2
// output till ^BFD     :     Warning
(BFD     :     Loading debug symbols from.*
)+
BFD     :     Warning: Address 0x[a-f0-9]+ in section \.text does not have a line number, perhaps the unit containing the function
              `main' wasn't compiled with flag -(g|gdb)
MALLOC  : <continued> 0x[a-f0-9]+
MALLOC  : Allocated memory: 400 bytes in 1 blocks\.
new\[\]     0x[a-f0-9]+             <unknown type>; \(sz = 400\) 
MALLOC  : delete\[\] 0x[a-f0-9]+             <unknown type>; \(sz = 400\)  
