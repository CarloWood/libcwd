// type regexp
BFD     : Loading debug symbols from .*/tst_bfd_s.....\.\.\. done \([0-9]+ symbols\)
// input lines 2
// output till ^BFD     : address
(BFD     : Loading debug symbols from .*\.so.* \(0x[0-9a-f]+\) \.\.\. (done \([0-9]+ symbols\)|No symbols found)
)+
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:60
NOTICE  : called from bfd\.cc:60
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:65
NOTICE  : called from bfd\.cc:65
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:70
NOTICE  : called from bfd\.cc:70
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:91
NOTICE  : called from bfd\.cc:91
// input lines 3
// output till ^BFD     : Warning
(BFD     : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)*
BFD     : Warning: Address 0x[0-9a-f]+ in section \.text does not have a line number, perhaps the unit containing the function
          `_*start' wasn't compiled with flag -(g|ggdb)
NOTICE  : called from <unknown location>
