// input lines 2
// output till ^COREDUMP
(STABS   : Loading debug info from.*
)*
COREDUMP: You are 'delete\[\]'-ing a pointer \(0x[0-9a-f]*\) with a corrupt second magic number \(buffer overrun\?\)!
