// input lines 3
// output till ^WARNING : Ok
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
WARNING : Ok
// input lines 2
// output till ^(q|BFD)
(WARNING : Object file /lib/libc\.so\.6 does not have debug info.*
)*
// input lines 2
// output till ^q
(BFD     : Loading debug info from .*/threads_keys_s.....\.\.\. done
)*
q = 0x[0-9a-f]*; \*q = 123454321
