// input lines 3
// output till ^(BFD     :     Warning|MALLOC)
((WARNING : core size is limited.*
)*(BFD     : Loading debug info from.*
)*)
MALLOC  : operator new\[\] \(size = 400\) = <unfinished>
BFD     :     Warning: Address 0x[a-f0-9]+ in section \.text of object file "nodebug_nodebug_(shared|static)"
              does not have a line number, perhaps the unit containing the function
              `_*main' wasn't compiled with flag -(g|ggdb)\?
MALLOC  : <continued> 0x[a-f0-9]+
MALLOC  : Allocated memory: (400|408) bytes in (1|2) blocks\.
new\[\]     0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\) 
// input lines 2
// output till ^MALLOC
(malloc    0x[a-f0-9]+ *eh_globals\.cc:[0-9]* *<unknown type>; \(sz = 8\) 
)*
MALLOC  : delete\[\] 0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\)  
