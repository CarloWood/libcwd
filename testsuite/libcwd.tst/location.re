// type regexp
// input lines 4
// output till ^BFD     : (address|Loading debug info)
((WARNING : core size is limited.*
)*(BFD     : Loading debug symbols from .*/tst_location_s.....\.\.\. done \([0-9]+ symbols\)
(BFD     : Loading debug symbols from (.*\.so.* \(0x[0-9a-f]+\)|/usr/lib/libdl\.so\.1) \.\.\. (done \([0-9]+ symbols\)|No symbols found)
)+)*)
BFD     : Loading debug info from .*/testsuite/tst_location_s.....\.\.\. done
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:110
NOTICE  : called from location\.cc:110
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:118
NOTICE  : called from location\.cc:118
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:126
NOTICE  : called from location\.cc:126
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:158
NOTICE  : called from location\.cc:158
// input lines 3
// output till ^(WARNING|NOTICE  : Program end)
(BFD     : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)*
// input lines 3
// output till ^(BFD|NOTICE  : Program end)
(WARNING : Object file .*/libc\.so\.6 does not have debug info.*
NOTICE  : called from <unknown location>
)*
// input lines 5
// output till ^NOTICE  : Program end
(BFD     : Warning: Address 0x[0-9a-f]+ in section \.text of object file "[^"]*"
          does not have a line number, perhaps the unit containing the function
          `(_*start|__libc_start_main)' wasn't compiled with flag -(g|ggdb)\?
NOTICE  : called from <unknown location>
)*
NOTICE  : Program end
