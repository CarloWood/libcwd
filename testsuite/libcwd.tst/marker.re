MALLOC  : operator new \(size = 12\) = 0x[0-9a-f]*
MALLOC  : operator new \(size = 1\) = 0x[0-9a-f]*
MALLOC  : New debugmalloc_marker_ct at 0x[0-9a-f]*
MALLOC  : operator new\[\] \(size = 120\) = 0x[0-9a-f]*
MALLOC  : operator new\[\] \(size = 120\) = 0x[0-9a-f]*
MALLOC  : Allocated memory: 253 bytes in 4 blocks\.
\(MARKER\)  0x[0-9a-f]*            marker\.cc:52   <marker>; \(sz = 1\)  A test marker
    new\[\]     0x[0-9a-f]*            marker\.cc:57   int\[30\]; \(sz = 120\)  Created after the marker
    new\[\]     0x[0-9a-f]*            marker\.cc:55   A\[10\]; \(sz = 120\)  Created after the marker
          0x[0-9a-f]*            marker\.cc:48   A; \(sz = 12\)  First created
NOTICE  : Moving the int array outside of the marker\.\.\.
MALLOC  : Allocated memory: 253 bytes in 4 blocks\.
new\[\]     0x[0-9a-f]*            marker\.cc:57   int\[30\]; \(sz = 120\)  Created after the marker
\(MARKER\)  0x[0-9a-f]*            marker\.cc:52   <marker>; \(sz = 1\)  A test marker
    new\[\]     0x[0-9a-f]*            marker\.cc:55   A\[10\]; \(sz = 120\)  Created after the marker
          0x[0-9a-f]*            marker\.cc:48   A; \(sz = 12\)  First created
MALLOC  : Removing debugmalloc_marker_ct at 0x[0-9a-f]* \(A test marker\)
  \* WARNING : Memory leak detected!
  \* new\[\]     0x[0-9a-f]*            marker\.cc:55   A\[10\]; \(sz = 120\)  Created after the marker
MALLOC  : delete 0x[0-9a-f]*            marker\.cc:52   <marker>; \(sz = 1\)  A test marker 
MALLOC  : delete\[\] 0x[0-9a-f]*            marker\.cc:57   int\[30\]; \(sz = 120\)  Created after the marker 
MALLOC  : delete\[\] 0x[0-9a-f]*            marker\.cc:55   A\[10\]; \(sz = 120\)  Created after the marker 
MALLOC  : delete 0x[0-9a-f]*            marker\.cc:48   A; \(sz = 12\)  First created 
NOTICE  : Finished successfully\.
