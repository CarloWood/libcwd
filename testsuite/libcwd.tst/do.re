// input lines 3
// output till ^NOTICE
((WARNING : core size is limited.*
)*BFD     : Loading debug info from.*
)*
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
* NOTICE  : This is written to cout
> NOTICE  : This is written to cerr
* NOTICE  : This is written to cerr
> WARNING : Was written to ostringstream: "* NOTICE  : This is written to an ostringstream
"
