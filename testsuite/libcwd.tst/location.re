// input lines 3
// output till ^BFD     : address
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
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
