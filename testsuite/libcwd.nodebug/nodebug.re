// input lines 2
// output till ^(STABS   :     Warning|MALLOC)
(STABS   : Loading debug info from.*
)*
MALLOC  : operator new\[\] \(size = 400\) = <unfinished>
STABS   :     Warning: Address 0x[a-f0-9]+ in section \.text does not have a line number, perhaps the unit containing the function
              `_*main' wasn't compiled with flag -(g|ggdb)
MALLOC  : <continued> 0x[a-f0-9]+
MALLOC  : Allocated memory: 400 bytes in 1 blocks\.
new\[\]     0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\) 
MALLOC  : delete\[\] 0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\)  
