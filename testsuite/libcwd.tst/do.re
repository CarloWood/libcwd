// input lines 3
// output till ^NOTICE
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
// type exact
NOTICE  : Dout Turned on 1
DEBUG   : Dout Turned on 2
DEBUG   : MyOwnDout Turned on 1
NOTICE  : MyOwnDout Turned on 2
NOTICE  : ExampleDout Turned on 1
DEBUG   : ExampleDout 2
DEBUG   : ExampleDout 3
DEBUG   : ExampleDout 4
DEBUG   : ExampleDout 5
DEBUG   : ExampleDout 6
NOTICE  : ExampleDout 7
NOTICE  : ExampleDout 8
NOTICE  : ExampleDout 9
NOTICE  : ExampleDout 10
NOTICE  : MyOwnDout 3
NOTICE  : ExampleDout 11
DEBUG   : 2311
***********libcw_do*DEBUG   |marker1|No indent
***********libcw_do*NOTICE  |marker1|No indent
***********libcw_do*WARNING |marker1|No indent
***********libcw_do*WARNING |marker1|        Dout text 8, "***********libcw_do*", "|marker1|".
**********my_own_do*WARNING |marker2|           MyOwnDout text 11, "**********my_own_do*", "|marker2|".
*example::my_own_do*WARNING |marker3|             ExampleDout text 13, "*example::my_own_do*", "|marker3|".
***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, "***********libcw_do*1Alibcw_do1", "|marker1|".
**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "**********my_own_do*1Amy_own_do1", "|marker2|".
*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text 13, "*example::my_own_do*1Aexample::my_own_do1", "|marker3|".
1Plibcw_do1***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, "1Plibcw_do1***********libcw_do*1Alibcw_do1", "|marker1|".
1Pmy_own_do1**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "1Pmy_own_do1**********my_own_do*1Amy_own_do1", "|marker2|".
1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text 13, "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1", "|marker3|".
WARNING |marker1|        Dout text 8, "", "|marker1|".
*WARNING |marker2|           MyOwnDout text 11, "*", "|marker2|".
XYZWARNING |marker3|             ExampleDout text 13, "XYZ", "|marker3|".
2Alibcw_do2WARNING |marker1|        Dout text 8, "2Alibcw_do2", "|marker1|".
*2Amy_own_do2WARNING |marker2|           MyOwnDout text 11, "*2Amy_own_do2", "|marker2|".
XYZ2Aexample::my_own_do2WARNING |marker3|             ExampleDout text 13, "XYZ2Aexample::my_own_do2", "|marker3|".
1Plibcw_do1***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, "1Plibcw_do1***********libcw_do*1Alibcw_do1", "|marker1|".
1Pmy_own_do1**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "1Pmy_own_do1**********my_own_do*1Amy_own_do1", "|marker2|".
1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text 13, "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1", "|marker3|".
1Plibcw_do1***********libcw_do*1Alibcw_do13Alibcw_do3WARNING |marker1|        Dout text 8, "1Plibcw_do1***********libcw_do*1Alibcw_do13Alibcw_do3", "|marker1|".
1Pmy_own_do1**********my_own_do*1Amy_own_do13Amy_own_do3WARNING |marker2|           MyOwnDout text 11, "1Pmy_own_do1**********my_own_do*1Amy_own_do13Amy_own_do3", "|marker2|".
1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do13Aexample::my_own_do3WARNING |marker3|             ExampleDout text 13, "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do13Aexample::my_own_do3", "|marker3|".
***********libcw_do*WARNING |marker1|        Dout text 8, "***********libcw_do*", "|marker1|".
**********my_own_do*WARNING |marker2|           MyOwnDout text 11, "**********my_own_do*", "|marker2|".
*example::my_own_do*WARNING |marker3|             ExampleDout text 13, "*example::my_own_do*", "|marker3|".
// type regexp
// input lines 2
// output till ^\* NOTICE  : This is written to cout
(> BFD     : Loading debug info from .*/libstdc\+\+\.so\.6\.\.\. done
)*
\* NOTICE  : This is written to cout
// type exact
> NOTICE  : This is written to cerr
* NOTICE  : This is written to cerr
> WARNING : Was written to ostringstream: "* NOTICE  : This is written to an ostringstream
"
