// type regexp
// input lines 3
// output till ^STABS   : address
(STABS   : Loading debug info from .*/tst_bfd_s.....\.\.\. done \([0-9]+ symbols\)
(STABS   : Loading debug info from (.*\.so.* \(0x[0-9a-f]+\)|/usr/lib/libdl\.so\.1) \.\.\. (done \([0-9]+ symbols\)|No symbols found)
)+)*
STABS   : address 0x[0-9a-f]+ corresponds to bfd\.cc:111
NOTICE  : called from bfd\.cc:111
STABS   : address 0x[0-9a-f]+ corresponds to bfd\.cc:119
NOTICE  : called from bfd\.cc:119
STABS   : address 0x[0-9a-f]+ corresponds to bfd\.cc:127
NOTICE  : called from bfd\.cc:127
STABS   : address 0x[0-9a-f]+ corresponds to bfd\.cc:159
NOTICE  : called from bfd\.cc:159
// input lines 3
// output till ^STABS   : Warning
(STABS   : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)*
STABS   : Warning: Address 0x[0-9a-f]+ in section \.text does not have a line number, perhaps the unit containing the function
          `_*start' wasn't compiled with flag -(g|ggdb)
NOTICE  : called from <unknown location>
