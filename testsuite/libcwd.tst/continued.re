// input lines 3
// output till ^BAR     : Enabled
((WARNING : core size is limited.*
)*(BFD     : Loading debug .*
)*)
BAR     : Enabled
// input lines 2
// output till ^DEBUG
(BFD     : Disabled
)?
DEBUG   : Disabled
FOO     : Enabled
MALLOC  : Disabled
NOTICE  : Enabled
RUN     : Enabled
SYSTEM  : Enabled
WARNING : Disabled
===========================================================================
 Unnested tests

---------------------------------------------------------------------------
NOTICE  : This is a single line
---------------------------------------------------------------------------
NOTICE  : This is a single line
---------------------------------------------------------------------------
NOTICE  : This is a single line with an error message behind it: 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
---------------------------------------------------------------------------
// input lines 3
// output till ==========
((\[31mBFD     :     Loading debug info from .*/libstdc\+\+\.so\.6\.\.\. done
\[0m)*\[31mNOTICE  : CERR: This is a single line with an error message behind it written to cerr: 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
)
\[0m===========================================================================
 Simple nests

---------------------------------------------------------------------------
FOO     :     Inside `nested_foo\(\)'
NOTICE  : `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
---------------------------------------------------------------------------
\[31mFOO     :     CERR: Inside `nested_foo\(\)'
\[0mNOTICE  : `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
---------------------------------------------------------------------------
FOO     :     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  : `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
---------------------------------------------------------------------------
\[31mFOO     :     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  : `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)'
FOO     :         Inside `nested_foo\(\)'
BAR     :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
BAR     :     Leaving `nested_bar\(\)'
NOTICE  : `nested_bar\(0, 0, 0, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)'
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mBAR     :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
BAR     :     Leaving `nested_bar\(\)'
NOTICE  : `nested_bar\(0, 0, 0, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)'
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
BAR     :     Leaving `nested_bar\(\)'
NOTICE  : `nested_bar\(0, 0, 1, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)'
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
BAR     :     Leaving `nested_bar\(\)'
NOTICE  : `nested_bar\(0, 0, 1, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :         Inside `nested_foo\(\)'
\[0m\[31mBAR     :     CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  : `nested_bar\(0, 1, 0, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :     CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  : `nested_bar\(0, 1, 0, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :     CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  : `nested_bar\(0, 1, 1, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :     CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  : `nested_bar\(0, 1, 1, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :         Inside `nested_foo\(\)'
BAR     :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :     Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  : `nested_bar\(1, 0, 0, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mBAR     :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :     Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  : `nested_bar\(1, 0, 0, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :     Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  : `nested_bar\(1, 0, 1, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
BAR     :     Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :     Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  : `nested_bar\(1, 0, 1, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :         Inside `nested_foo\(\)'
\[0m\[31mBAR     :     CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  : `nested_bar\(1, 1, 0, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :     CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  : `nested_bar\(1, 1, 0, 1\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :     CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  : `nested_bar\(1, 1, 1, 0\)' returns the string "Bar" when I call it\.
---------------------------------------------------------------------------
\[31mBAR     :     CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :     CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :     CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  : `nested_bar\(1, 1, 1, 1\)' returns the string "Bar" when I call it\.
===========================================================================
 Continued tests, single depth

---------------------------------------------------------------------------
RUN     : Hello World
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :     Single interruption before\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
FOO     :     Single interruption after\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :     Single interruption before and
RUN     : <continued> is an awesome <unfinished>
FOO     :     a single interruption after\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :     Double interruption before,
FOO     :     double means two lines\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
FOO     :     Double interruption after,
FOO     :     double means two lines\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :     Double interruption before and
FOO     :     \(double means two lines\)
RUN     : <continued> is an awesome <unfinished>
FOO     :     a double interruption after
FOO     :     \(double means two lines\)
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)'
NOTICE  :     `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)'
\[0mNOTICE  :     `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
FOO     :         Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
NOTICE  :     `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> is an awesome <unfinished>
\[31mFOO     :         CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mNOTICE  :     `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)'
FOO     :             Inside `nested_foo\(\)'
BAR     :         `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)'
\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0mBAR     :         `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)'
FOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :         `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)'
\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :         `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :             Inside `nested_foo\(\)'
BAR     :         `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0mBAR     :         `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :         `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :         `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> is an awesome library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)'
FOO     :             Inside `nested_foo\(\)'
BAR     :         `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)'
\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0mBAR     :         `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)'
FOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :         `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)'
\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :         `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
BAR     :         Leaving `nested_bar\(\)'
NOTICE  :     `nested_bar\(0, 0, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)'
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)'
\[0mNOTICE  :     `nested_bar\(0, 1, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :             Inside `nested_foo\(\)'
BAR     :         `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0mBAR     :         `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
FOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
BAR     :         `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
BAR     :         Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mBAR     :         `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
BAR     :         Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
NOTICE  :     `nested_bar\(1, 0, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 0, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)'
\[0m\[31mBAR     :         CERR: `nested_foo\(0, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 0, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 0\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 1, 0\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
---------------------------------------------------------------------------
RUN     : Libcwd is an awesome <unfinished>
\[31mBAR     :         CERR: Entering `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0m\[31mFOO     :             CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0m\[31mBAR     :         CERR: `nested_foo\(1, 1\)' returns the string "Foo" when I call it\.: EINVAL \(Invalid argument\)
\[0m\[31mBAR     :         CERR: Leaving `nested_bar\(\)': EINVAL \(Invalid argument\)
\[0mNOTICE  :     `nested_bar\(1, 1, 1, 1\)' returns the string "Bar" when I call it\.
RUN     : <continued> library!
===========================================================================
 Continued tests, deep

RUN     :     1<unfinished>
FOO     :             Inside `nested_foo\(\)'
RUN     :             1<unfinished>
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mRUN     :                     1<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 2<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 3
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mFOO     :                 Foo3:3Foo
RUN     :             <continued> 2<unfinished>
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mRUN     :                     1<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 2<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 3
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mFOO     :                 Foo3:3Foo
RUN     :             <continued> 3
FOO     :             Inside `nested_foo\(\)'
FOO     :         Foo4:4Foo
RUN     :     <continued> 2<unfinished>
FOO     :             Inside `nested_foo\(\)'
RUN     :             1<unfinished>
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mRUN     :                     1<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 2<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 3
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mFOO     :                 Foo3:3Foo
RUN     :             <continued> 2<unfinished>
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mRUN     :                     1<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 2<unfinished>
FOO     :                             Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
RUN     :                             1<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 2<unfinished>
\[31mFOO     :                                     CERR: Inside `nested_foo\(\)'
\[0mFOO     :                                     Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
FOO     :                                 Foo1BOTTOM1Foo
RUN     :                             <continued> 3
\[31mFOO     :                             CERR: Inside `nested_foo\(\)'
\[0mFOO     :                         Foo2:2Foo
RUN     :                     <continued> 3
\[31mFOO     :                     CERR: Inside `nested_foo\(\)': 0 \((Success|Error 0|Undefined error: 0|Unknown error: 0)\)
\[0mFOO     :                 Foo3:3Foo
RUN     :             <continued> 3
FOO     :             Inside `nested_foo\(\)'
FOO     :         Foo4:4Foo
RUN     :     <continued> 3
NOTICE  : :
