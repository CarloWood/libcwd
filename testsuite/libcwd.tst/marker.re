// input lines 3
// output till ^MALLOC
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
MALLOC  : operator new \(size = 12\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 8\) = 0x[0-9a-f]*
MALLOC  : New libcwd::marker_ct at 0x[0-9a-f]*
MALLOC  : operator new\[\] \(size = 120\) = 0x[0-9a-f]*
MALLOC  : operator new\[\] \(size = 120\) = 0x[0-9a-f]*
MALLOC  : Allocated memory: 260 bytes in 4 blocks\.
\(MARKER\)  0x[0-9a-f]*            marker\.cc:59   <marker>; \(sz = 8\)  A test marker
    new\[\]     0x[0-9a-f]*            marker\.cc:65   int\[30\]; \(sz = 120\)  Created after the marker
    new\[\]     0x[0-9a-f]*            marker\.cc:63   Am\[10\]; \(sz = 120\)  Created after the marker
          0x[0-9a-f]*            marker\.cc:54   Am; \(sz = 12\)  First created
NOTICE  : Moving the int array outside of the marker\.\.\.
MALLOC  : Allocated memory: 260 bytes in 4 blocks\.
new\[\]     0x[0-9a-f]*            marker\.cc:65   int\[30\]; \(sz = 120\)  Created after the marker
\(MARKER\)  0x[0-9a-f]*            marker\.cc:59   <marker>; \(sz = 8\)  A test marker
    new\[\]     0x[0-9a-f]*            marker\.cc:63   Am\[10\]; \(sz = 120\)  Created after the marker
          0x[0-9a-f]*            marker\.cc:54   Am; \(sz = 12\)  First created
MALLOC  : Removing libcwd::marker_ct at 0x[0-9a-f]* \(A test marker\)
  \* WARNING : Memory leak detected!
  \* new\[\]     0x[0-9a-f]*            marker\.cc:63   Am\[10\]; \(sz = 120\)  Created after the marker
MALLOC  : delete 0x[0-9a-f]*            marker\.cc:59   <marker>; \(sz = 8\)  A test marker 
MALLOC  : delete\[\] 0x[0-9a-f]*            marker\.cc:65   int\[30\]; \(sz = 120\)  Created after the marker 
MALLOC  : delete\[\] 0x[0-9a-f]*            marker\.cc:63   Am\[10\]; \(sz = 120\)  Created after the marker 
MALLOC  : delete 0x[0-9a-f]*            marker\.cc:54   Am; \(sz = 12\)  First created 
NOTICE  : Finished successfully\.
