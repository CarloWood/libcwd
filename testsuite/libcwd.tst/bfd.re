// type regexp
BFD     : Loading debug symbols from .*/bfd\.\.\. done \([0-9]+ symbols\)
// input lines 2
// output till ^BFD     : address
(BFD     : Loading debug symbols from .*\.so.* \(0x[0-9a-f]+\) \.\.\. done \([0-9]+ symbols\)
)+
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:59
NOTICE  : called from bfd\.cc:59
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:64
NOTICE  : called from bfd\.cc:64
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:69
NOTICE  : called from bfd\.cc:69
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:90
NOTICE  : called from bfd\.cc:90
// input lines 3
// output till ^BFD     : Warning
(BFD     : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)+
BFD     : Warning: Address 0x[0-9a-f]+ in section \.text does not have a line number, perhaps the unit containing the function
          `_start' wasn't compiled with flag -(g|ggdb)
NOTICE  : called from <unknown location>
