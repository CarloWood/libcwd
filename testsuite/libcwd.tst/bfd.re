// type regexp
BFD     : Loading debug symbols from .*/tst_bfd_s.....\.\.\. done \([0-9]+ symbols\)
// input lines 2
// output till ^BFD     : address
(BFD     : Loading debug symbols from (.*\.so.* \(0x[0-9a-f]+\)|/usr/lib/libdl\.so\.1) \.\.\. (done \([0-9]+ symbols\)|No symbols found)
)+
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:109
NOTICE  : called from bfd\.cc:109
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:117
NOTICE  : called from bfd\.cc:117
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:125
NOTICE  : called from bfd\.cc:125
BFD     : address 0x[0-9a-f]+ corresponds to bfd\.cc:151
NOTICE  : called from bfd\.cc:151
// input lines 3
// output till ^BFD     : Warning
(BFD     : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)*
BFD     : Warning: Address 0x[0-9a-f]+ in section \.text does not have a line number, perhaps the unit containing the function
          `_*start' wasn't compiled with flag -(g|ggdb)
NOTICE  : called from <unknown location>
