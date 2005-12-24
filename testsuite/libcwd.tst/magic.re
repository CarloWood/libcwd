// input lines 3
// output till ^COREDUMP
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
COREDUMP: You are 'delete\[\]'-ing a pointer \(0x[0-9a-f]*\) with a corrupt (second magic number|redzone postfix) \(buffer overrun\?\)!
