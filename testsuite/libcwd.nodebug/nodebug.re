// input lines 5
// output till (Loading debug info.*/nodebug_nodebug_s|Warning: Address|Address lookups inside this object)
((WARNING : core size is limited.*
)*(BFD     : Loading debug symbols from.*
)*(BFD     : Loading debug info from .*/libcwd\.so\.0\.\.\. done
)*MALLOC  : operator new\[\] \(size = 400\) = <unfinished>
)
// input lines 6
// output till ^MALLOC
((BFD     :     Loading debug info from.*/nodebug_nodebug_s.....\.\.\. done
)*(BFD     :     Warning: Address 0x[a-f0-9]+ in section \.text of object file "nodebug_nodebug_(shared|static)"
              does not have a line number, perhaps the unit containing the function
              `_*main' wasn't compiled with flag -(g|ggdb)\?
)|(WARNING :     Object file .*/nodebug_nodebug_shared does not have debug info\.  Address lookups inside this object file will result in a function name only, not a source file location\.
))
MALLOC  : <continued> 0x[a-f0-9]+
MALLOC  : Allocated memory: (400|408) bytes in (1|2) blocks:
new\[\]     0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\) 
// input lines 2
// output till ^MALLOC
(malloc    0x[a-f0-9]+ *eh_globals\.cc:[0-9]* *<unknown type>; \(sz = 8\) 
)*
MALLOC  : delete\[\] 0x[a-f0-9]+ (_*main|) *<unknown type>; \(sz = 400\)  
