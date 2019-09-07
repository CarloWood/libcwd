// input lines 3
// output till ^BFD     : address
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:117
// input lines 2
// output till ^NOTICE
(WARNING :     Object file (/usr/lib/debug)*/lib/(x86_64-linux-gnu/)*libc-2.[.0-9]*.so does not have debug info.*
)*
NOTICE  : called from tst_location_shared:_Z16libcwd_bfd_test2v:location\.cc:117
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:126
NOTICE  : called from tst_location_shared:_Z16libcwd_bfd_test1v:location\.cc:126
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:135
NOTICE  : called from tst_location_shared:_Z15libcwd_bfd_testv:location\.cc:135
BFD     : address 0x[0-9a-f]+ corresponds to location\.cc:172
NOTICE  : called from tst_location_shared:main:location\.cc:172
// input lines 2
// output till ^BFD     : address
(BFD     : Loading debug info from /usr/lib/debug/lib/(x86_64-linux-gnu/)*libc-2\.[.0-9]*\.so\.\.\. done
)*
// input lines 3
// output till ^(WARNING|NOTICE  : Program end)
(BFD     : address 0x[0-9a-f]+ corresponds to .*
NOTICE  : called from .*
)*
// input lines 3
// output till ^(BFD|NOTICE  : Program end)
((WARNING : Object file .*/libc-2.[.0-9]*.so does not have debug info.*
)*NOTICE  : called from libc\.so\.6:__libc_start_main
)*
// input lines 5
// output till ^NOTICE  : Program end
(BFD     : Warning: Address 0x[0-9a-f]+ in section \.text of object file "[^"]*"
          does not have a line number, perhaps the unit containing the function
          `(_*start|__libc_start_main)' wasn't compiled with flag -(g|ggdb)\?
NOTICE  : called from libc\.so\.6:__libc_start_main
)*
NOTICE  : Program end
